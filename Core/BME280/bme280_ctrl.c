/**
 * @file bme280_ctrl.c
 *
 * Created on 14.07.2026.
 * @author Marek Godlowicz https://github.com/MarekGodlo
 * @brief Non-blocking driver for the BME280 sensor, built on top of the
 *       Bosch BME280 API, exposing a polling-based state machine interface.
 */

#include "bme280_ctrl.h"

#include <stdbool.h>

#include "../Systim/systim.h"
#include "BME_Driver/bme280.h"

static BME280_Status_t start_measurement(BME280_Handle_t *hbme, uint32_t *req_delay);
static BME280_Status_t read_data_from_sensor(BME280_Handle_t *hbme);

static int8_t apply_setting_to_device(BME280_Handle_t *hbme, struct bme280_settings *settings);
static bool parse_data(const struct bme280_data *raw_data, BME280_Data_t *comp_data);
static void set_default_settings(BME280_Handle_t *hbme);

static BME280_Status_t handle_error(BME280_Handle_t *hbme);

static void delay_us(uint32_t period, void *intf_ptr);
static BME280_INTF_RET_TYPE write_data(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr);
static BME280_INTF_RET_TYPE read_data(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr);

BME280_Status_t BME280_Init(BME280_Handle_t *hbme, TIM_HandleTypeDef *htim, I2C_HandleTypeDef *hi2c, const uint8_t i2c_addr) {
    if (hbme == NULL) {
        return BME280_STATUS_NULL_PTR;
    }

    assert_param(hbme != NULL);
    assert_param(hi2c != NULL);
    assert_param(htim != NULL);

    hbme->intf.htim = htim;
    hbme->intf.hi2c = hi2c;
    hbme->intf.i2c_addr = i2c_addr;

    hbme->dev.delay_us = delay_us;
    hbme->dev.write = write_data;
    hbme->dev.read = read_data;
    hbme->dev.intf = BME280_I2C_INTF;
    hbme->dev.intf_ptr = &hbme->intf;

    hbme->ctx.state = BME280_STATE_RESET;

    set_default_settings(hbme);

    const int8_t rslt = bme280_init(&hbme->dev);
    if (rslt != BME280_OK) return handle_error(hbme);

    hbme->ctx.state = BME280_STATE_IDLE;
    return BME280_STATUS_OK;
}

BME280_Status_t BME280_SetOversampling(BME280_Handle_t *hbme, const uint8_t osr_temp, const uint8_t osr_hum, const uint8_t osr_press, const uint8_t filter) {
    if (hbme == NULL) {
        return BME280_STATUS_NULL_PTR;
    }

    assert_param(hbme != NULL);

    hbme->config.osr_temp = osr_temp;
    hbme->config.osr_hum = osr_hum;
    hbme->config.osr_press = osr_press;
    hbme->config.filter = filter;
    hbme->config.dirty = true;
    return BME280_STATUS_OK;
}

BME280_Status_t BME280_StartMeasurement_Async(BME280_Handle_t *hbme) {
    if (hbme == NULL) {
        return BME280_STATUS_NULL_PTR;
    }

    assert_param(hbme != NULL);

    if (hbme->ctx.state == BME280_STATE_BUSY || hbme->ctx.phase == BME280_PHASE_DONE) return BME280_STATUS_BUSY;

    hbme->ctx.phase = BME280_PHASE_START_MEASUREMENT;
    hbme->ctx.state = BME280_STATE_BUSY;
    return BME280_STATUS_OK;
}

bool BME280_IsBusy(const BME280_Handle_t *hbme) {
    if (hbme == NULL) return false;
    assert_param(hbme != NULL);

    return hbme->ctx.state == BME280_STATE_BUSY;
}

bool BME280_IsDataReady(const BME280_Handle_t *hbme) {
    if (hbme == NULL) return false;
    assert_param(hbme != NULL);

    return hbme->ctx.phase == BME280_PHASE_DONE;
}

BME280_Status_t BME280_GetData(BME280_Handle_t *hbme, BME280_Data_t *data) {
    if (hbme == NULL || data == NULL) {
        return BME280_STATUS_NULL_PTR;
    }
    assert_param(hbme != NULL);
    assert_param(data != NULL);

    if (hbme->ctx.phase != BME280_PHASE_DONE) return BME280_STATUS_BUSY;

    *data = hbme->data;
    hbme->ctx.phase = BME280_PHASE_IDLE;
    return BME280_STATUS_OK;
}

BME280_Status_t BME280_Task(BME280_Handle_t *hbme) {
    if (hbme == NULL) return BME280_STATUS_NULL_PTR;
    assert_param(hbme != NULL);

    switch (hbme->ctx.phase) {
        case BME280_PHASE_IDLE:
            // nothing to do
            break;
        case BME280_PHASE_START_MEASUREMENT:
            if (start_measurement(hbme, &hbme->ctx.deadline_us) != BME280_STATUS_OK) {
                return handle_error(hbme);
            }

            hbme->ctx.now_us = Systim_GetUs();
            hbme->ctx.phase = BME280_PHASE_WAIT;
            break;
        case BME280_PHASE_WAIT:
            if (Systim_GetUs() - hbme->ctx.now_us >= hbme->ctx.deadline_us) {
                hbme->ctx.phase = BME280_PHASE_READ_DATA;
            }
            break;
        case BME280_PHASE_READ_DATA:
            if (read_data_from_sensor(hbme) != BME280_STATUS_OK) {
                return handle_error(hbme);
            }

            hbme->ctx.phase = BME280_PHASE_DONE;
            // hbme->ctx.state is reset to IDLE by BME280_GetData(), not here.
            break;
        case BME280_PHASE_DONE:
            // nothing to do
            break;
        default:
            return handle_error(hbme);
    }

    return BME280_STATUS_OK;
}

static BME280_Status_t start_measurement(BME280_Handle_t *hbme, uint32_t *req_delay) {
    assert_param(hbme != NULL);
    assert_param(req_delay != NULL);

    struct bme280_settings settings;
    int8_t rslt;

    if (hbme->config.dirty) {
        // sync new settings to the sensor (I2C write)
        rslt = apply_setting_to_device(hbme, &settings);
        if (rslt != BME280_OK) return handle_error(hbme);
        hbme->config.dirty = false;
    } else {
        // reuse settings already applied to the sensor
        settings.osr_p = hbme->config.osr_press;
        settings.osr_h = hbme->config.osr_hum;
        settings.osr_t = hbme->config.osr_temp;
        settings.filter = hbme->config.filter;
    }

    rslt = bme280_set_sensor_mode(BME280_POWERMODE_FORCED, &hbme->dev);
    if (rslt != BME280_OK) return handle_error(hbme);

    rslt = bme280_cal_meas_delay(req_delay, &settings);
    if (rslt != BME280_OK) return handle_error(hbme);

    return BME280_STATUS_OK;
}

static BME280_Status_t read_data_from_sensor(BME280_Handle_t *hbme) {
    assert_param(hbme != NULL);

    struct bme280_data data;
    int8_t rslt;

    rslt = bme280_get_sensor_data(BME280_ALL, &data, &hbme->dev);

    if (rslt != BME280_OK) return handle_error(hbme);

    if (!parse_data(&data, &hbme->data)) return handle_error(hbme);

    hbme->ctx.state = BME280_STATE_IDLE;
    return BME280_STATUS_OK;
}

static int8_t apply_setting_to_device(BME280_Handle_t *hbme, struct bme280_settings *settings) {
    assert_param(hbme != NULL);
    assert_param(settings != NULL);

    settings->osr_p = hbme->config.osr_press;
    settings->osr_h = hbme->config.osr_hum;
    settings->osr_t = hbme->config.osr_temp;
    settings->filter = hbme->config.filter;

    uint8_t desired_settings = 0;
    
    if (settings->osr_p != BME280_NO_OVERSAMPLING) {
        desired_settings |= BME280_SEL_OSR_PRESS;
    }

    if (settings->osr_h != BME280_NO_OVERSAMPLING) {
        desired_settings |= BME280_SEL_OSR_HUM;
    }

    if (settings->osr_t != BME280_NO_OVERSAMPLING) {
        desired_settings |= BME280_SEL_OSR_TEMP;
    }

    if (settings->filter != BME280_FILTER_COEFF_OFF) {
        desired_settings |= BME280_SEL_FILTER;
    }

    return bme280_set_sensor_settings(desired_settings, settings, &hbme->dev);
}

static bool parse_data(const struct bme280_data *raw_data, BME280_Data_t *comp_data) {
    // prevents unexpected errors; params may come from external library
    if (raw_data == NULL || comp_data == NULL) return false;
    assert_param(raw_data != NULL);
    assert_param(comp_data != NULL);

    comp_data->pressure = raw_data->pressure;
    comp_data->humidity = raw_data->humidity;
    comp_data->temperature = raw_data->temperature;

    return true;
}

static void set_default_settings(BME280_Handle_t *hbme) {
    assert_param(hbme != NULL);

    hbme->config.osr_press = BME280_OVERSAMPLING_1X;
    hbme->config.osr_hum = BME280_OVERSAMPLING_1X;
    hbme->config.osr_temp = BME280_OVERSAMPLING_1X;
    hbme->config.filter = BME280_FILTER_COEFF_OFF;
    hbme->config.dirty = true;
}

static BME280_Status_t handle_error(BME280_Handle_t *hbme) {
    assert_param(hbme != NULL);

    hbme->ctx.state = BME280_STATE_ERROR;
    return BME280_STATUS_ERROR;
}

/** @name Low-Level hardware interface (BME280 driver callbacks) **/
//@{
static void delay_us(uint32_t period, void *intf_ptr) {
    assert_param(intf_ptr != NULL);

    const BME280_Intf_t *intf = intf_ptr;

    HAL_TIM_Base_Start(intf->htim);
    __HAL_TIM_SET_COUNTER(intf->htim, 0);
    while (__HAL_TIM_GET_COUNTER(intf->htim) < period) {
        __WFE();
    }
    HAL_TIM_Base_Stop(intf->htim);
}


static BME280_INTF_RET_TYPE write_data(const uint8_t reg_addr, const uint8_t *reg_data, const uint32_t len, void *intf_ptr) {
    assert_param(reg_data != NULL);
    assert_param(intf_ptr != NULL);

    const BME280_Intf_t *intf = (BME280_Intf_t *)intf_ptr;

    const HAL_StatusTypeDef status = HAL_I2C_Mem_Write(intf->hi2c, intf->i2c_addr, reg_addr, I2C_MEMADD_SIZE_8BIT, reg_data, len, 100);
    return status == HAL_OK ? BME280_OK : BME280_E_COMM_FAIL;
}

static BME280_INTF_RET_TYPE read_data(const uint8_t reg_addr, uint8_t *reg_data, const uint32_t len, void *intf_ptr) {
    assert_param(reg_data != NULL);
    assert_param(intf_ptr != NULL);

    const BME280_Intf_t *intf = (BME280_Intf_t *)intf_ptr;

    const HAL_StatusTypeDef status = HAL_I2C_Mem_Read(intf->hi2c, intf->i2c_addr, reg_addr, I2C_MEMADD_SIZE_8BIT, reg_data, len, 100);
    return status == HAL_OK ? BME280_OK : BME280_E_COMM_FAIL;
}
//@}