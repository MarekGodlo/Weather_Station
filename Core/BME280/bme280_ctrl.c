/**
 * @file bme280_ctrl.c
 *
 * Created on 14.07.2026.
 * @author Marek Godlowicz https://github.com/MarekGodlo
 * @brief Non-blocking driver for the BME280 sensor, built on top of the
 *        Bosch BME280 API, exposing a polling-based state machine interface.
 */

#include "bme280_ctrl.h"

#include <stdbool.h>

#include "../Systim/systim.h"
#include "BME_Driver/bme280.h"

static void handle_start_meas_phase(BME280_Handle_t *hbme);
static void handle_wait_phase(BME280_Handle_t *hbme);
static void handle_read_data_phase(BME280_Handle_t *hbme);
static void handle_error_phase(BME280_Handle_t *hbme);

static BME280_Error_t start_measurement(BME280_Handle_t *hbme, uint32_t *req_delay);
static BME280_Error_t read_data_from_sensor(BME280_Handle_t *hbme);

static int8_t apply_setting_to_device(BME280_Handle_t *hbme, struct bme280_settings *settings);
static bool parse_data(const struct bme280_data *raw_data, BME280_Data_t *comp_data);

// Low-Level hardware interface
static void delay_us(uint32_t period, void *intf_ptr);
static BME280_INTF_RET_TYPE write_data(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr);
static BME280_INTF_RET_TYPE read_data(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr);

BME280_Status_t BME280_Init(BME280_Handle_t *hbme, const BME280_Config_t *config, const BME280_Intf_t *intf) {
    assert_param(hbme != NULL);
    assert_param(config != NULL);
    assert_param(intf != NULL);

    if (hbme == NULL || config == NULL || intf == NULL) {
        return BME280_STATUS_NULL_ARG;
    }

    hbme->intf = *intf;
    hbme->config = *config;

    // configures library struct
    hbme->dev.delay_us = delay_us;
    hbme->dev.write = write_data;
    hbme->dev.read = read_data;
    hbme->dev.intf = BME280_I2C_INTF;
    hbme->dev.intf_ptr = &hbme->intf;

    const int8_t rslt = bme280_init(&hbme->dev);
    if (rslt != BME280_OK) {
        return BME280_STATUS_ERROR;
    }

    // sets default values
    hbme->ctx.is_busy = false;
    hbme->ctx.phase = BME280_PHASE_IDLE;
    hbme->ctx.now_us = 0;
    hbme->ctx.deadline_us = 0;
    return BME280_STATUS_OK;
}

BME280_Status_t BME280_StartMeas_Async(BME280_Handle_t *hbme) {
    assert_param(hbme != NULL);

    if (hbme == NULL) {
        return BME280_STATUS_NULL_ARG;
    }

    if (hbme->ctx.is_busy) {
        return BME280_STATUS_BUSY;
    }

    hbme->ctx.phase = BME280_PHASE_START_MEASUREMENT;
    hbme->ctx.is_busy = true;
    return BME280_STATUS_OK;
}

bool BME280_IsDataReady(const BME280_Handle_t *hbme) {
    assert_param(hbme != NULL);

    if (hbme == NULL) {
        return false;
    }

    return hbme->ctx.phase == BME280_PHASE_DONE;
}

BME280_Status_t BME280_GetData(BME280_Handle_t *hbme, BME280_Data_t *data) {
    assert_param(hbme != NULL);
    assert_param(data != NULL);

    if (hbme == NULL || data == NULL) {
        return BME280_STATUS_NULL_ARG;
    }

    *data = hbme->data;
    hbme->ctx.phase = BME280_PHASE_IDLE;
    hbme->ctx.is_busy = false;
    return BME280_STATUS_OK;
}

bool BME280_HasError(const BME280_Handle_t *hbme) {
    if (hbme == NULL) return false;

    return hbme->ctx.phase == BME280_PHASE_ERROR;
}

BME280_Error_t BME280_GetError(const BME280_Handle_t *hbme) {
    if (hbme == NULL) return BME280_ERROR_NONE;

    return hbme->ctx.last_error;
}

BME280_Status_t BME280_ClearError(BME280_Handle_t *hbme) {
    assert_param(hbme != NULL);

    if (hbme == NULL) {
        return BME280_STATUS_NULL_ARG;
    }

    hbme->ctx.last_error = BME280_ERROR_NONE;
    hbme->ctx.phase = BME280_PHASE_IDLE;
    hbme->ctx.is_busy = false;
    return BME280_STATUS_OK;
}

void BME280_Task(BME280_Handle_t *hbme) {
    assert_param(hbme != NULL);

    switch (hbme->ctx.phase) {
        case BME280_PHASE_IDLE: // nothing to do
            break;
        case BME280_PHASE_START_MEASUREMENT: handle_start_meas_phase(hbme);
            break;
        case BME280_PHASE_WAIT: handle_wait_phase(hbme);
            break;
        case BME280_PHASE_READ_DATA: handle_read_data_phase(hbme);
            break;
        case BME280_PHASE_DONE: // nothing to do
            break;
        case BME280_PHASE_ERROR: handle_error_phase(hbme);
            break;
        default:
            hbme->ctx.last_error = BME280_ERROR_INVALID_PHASE;
            hbme->ctx.phase = BME280_PHASE_ERROR;
    }
}

static void handle_start_meas_phase(BME280_Handle_t *hbme) {
    assert_param(hbme != NULL);

    const BME280_Error_t error = start_measurement(hbme, &hbme->ctx.deadline_us);

    if (error != BME280_ERROR_NONE) {
        hbme->ctx.last_error = error;
        hbme->ctx.phase = BME280_PHASE_ERROR;
        return;
    }

    hbme->ctx.now_us = Systim_GetUs();
    hbme->ctx.phase = BME280_PHASE_WAIT;
}

static void handle_wait_phase(BME280_Handle_t *hbme) {
    assert_param(hbme != NULL);

    if (Systim_GetUs() - hbme->ctx.now_us >= hbme->ctx.deadline_us) {
        hbme->ctx.phase = BME280_PHASE_READ_DATA;
    }
}

static void handle_read_data_phase(BME280_Handle_t *hbme) {
    assert_param(hbme != NULL);

    const BME280_Error_t error = read_data_from_sensor(hbme);

    if (error != BME280_ERROR_NONE) {
        hbme->ctx.last_error = error;
        hbme->ctx.phase = BME280_PHASE_ERROR;
        return;
    }

    hbme->ctx.phase = BME280_PHASE_DONE;
}

static void handle_error_phase(BME280_Handle_t *hbme) {
    assert_param(hbme != NULL);

    hbme->ctx.is_busy = false;
}

static BME280_Error_t start_measurement(BME280_Handle_t *hbme, uint32_t *req_delay) {
    assert_param(hbme != NULL);
    assert_param(req_delay != NULL);

    struct bme280_settings settings;
    int8_t rslt;

    rslt = apply_setting_to_device(hbme, &settings);
    if (rslt != BME280_OK) {
        return BME280_ERROR_INVALID_CONFIG;
    }
    hbme->config.dirty = false;

    rslt = bme280_set_sensor_mode(BME280_POWERMODE_FORCED, &hbme->dev);
    if (rslt != BME280_OK) {
        return BME280_ERROR_COMM_FAIL;
    }

    rslt = bme280_cal_meas_delay(req_delay, &settings);
    if (rslt != BME280_OK) {
        return BME280_ERROR_COMM_FAIL;
    }

    return BME280_ERROR_NONE;
}

static BME280_Error_t read_data_from_sensor(BME280_Handle_t *hbme) {
    assert_param(hbme != NULL);

    struct bme280_data data;
    int8_t rslt;

    rslt = bme280_get_sensor_data(BME280_ALL, &data, &hbme->dev);
    if (rslt != BME280_OK) {
        return BME280_ERROR_COMM_FAIL;
    }

    if (!parse_data(&data, &hbme->data)) {
        return BME280_ERROR_PARSE_FAIL;
    }

    return BME280_ERROR_NONE;
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