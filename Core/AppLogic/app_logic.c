/**
 * @file app_logic.c
 *
 * Created on 9/20/26.
 * @author Marek Godlowicz https://github.com/MarekGodlo
 * @brief Provides the core logic and state machine for the weather station application.
 */

#include "app_logic.h"

#include "../Systim/systim.h"

static App_Status_t sensor_systim_init(TIM_HandleTypeDef *htim);
static App_Status_t sensor_bme280_init(BME280_Handle_t *hbme, const BME280_Intf_t *intf);
static App_Status_t sensor_ds18b20_init(DS18B20_Handle_t *hds, OneWire_Handle_t *how);
static App_Status_t ui_init(App_Handle_t *happ);

App_Status_t systim_run(App_Handle_t *happ);
App_Status_t ds18b20_run(App_Handle_t *happ);
App_Status_t bme280_run(App_Handle_t *happ);

static void handle_measure_phase(App_Handle_t *happ);
static void handle_render_phase(App_Handle_t *happ);
static void handle_sleep_phase();
static void handle_error_phase();
static void handle_default_phase(App_Handle_t *happ);

bool validate_config(const App_Config_t *config);

static void sensors_init(App_Handle_t *happ);

App_Status_t App_Init(App_Handle_t *happ, const App_Config_t *config) {
    assert_param(happ != NULL);
    assert_param(validate_config(config));

    if (happ == NULL || !validate_config(config)) {
        return APP_NULL_ARG;
    }

    happ->config = *config;
    happ->current_phase = APP_PHASE_INIT;
    happ->health.systim = APP_COMPONENT_OK;
    happ->health.bme280 = APP_COMPONENT_OK;
    happ->health.ds18b20 = APP_COMPONENT_OK;
    happ->health.epd = APP_COMPONENT_OK;

    sensors_init(happ);

    App_Status_t status = ui_init(happ);
    if (status != APP_OK) {
        happ->health.epd = APP_COMPONENT_ERROR;
    }

    const bool all_components_ok = (happ->health.systim == APP_COMPONENT_OK &&
                                    happ->health.bme280 == APP_COMPONENT_OK &&
                                    happ->health.ds18b20 == APP_COMPONENT_OK &&
                                    happ->health.epd == APP_COMPONENT_OK);

    return all_components_ok ? APP_OK : APP_MODULE_FAIL;
}

App_Status_t App_Run(App_Handle_t *happ) {
    assert_param(happ != NULL);

    if (happ == NULL) {
        return APP_NULL_ARG;
    }

    App_Status_t status;

    // Systim is critical: sensors cannot run without it started first
    status = systim_run(happ);
    if (status != APP_OK) {
        happ->current_phase = APP_PHASE_MEASURE;
        return status;
    }

    bool any_sensor_failed = false;

    if (bme280_run(happ) != APP_OK) {
        any_sensor_failed = true;
    }

    if (ds18b20_run(happ) != APP_OK) {
        any_sensor_failed = true;
    }

    happ->current_phase = APP_PHASE_MEASURE;
    return any_sensor_failed ? APP_MODULE_FAIL : APP_OK;
}


void App_Task(App_Handle_t *happ) {
    assert_param(happ != NULL);

    switch (happ->current_phase) {
        case APP_PHASE_MEASURE: handle_measure_phase(happ); break;
        case APP_PHASE_RENDER: handle_render_phase(happ); break;
        case APP_PHASE_SLEEP: handle_sleep_phase(); break;
        case APP_PHASE_ERROR: handle_error_phase(); break;
        default: handle_default_phase(happ); break;
    }
}

bool validate_config(const App_Config_t *config) {
    if (config == NULL) return false;
    if (config->systim_tim == NULL) return false;
    if (config->epd_pins == NULL) return false;
    if (config->bme280_intf == NULL) return false;
    if (config->hspi == NULL) return false;
    if (config->hpwr == NULL) return false;
    if (config->how == NULL) return false;
    if (config->retained == NULL) return false;
    if (config->gfx_buffer == NULL) return false;
    return true;
}

static void sensors_init(App_Handle_t *happ) {
    App_Status_t status;

    // DS18B20 and BME280 are dependent on this module
    status = sensor_systim_init(happ->config.systim_tim);
    if (status != APP_OK) {
        happ->health.systim = APP_COMPONENT_ERROR;
        happ->health.ds18b20 = APP_COMPONENT_ERROR;
        happ->health.bme280 = APP_COMPONENT_ERROR;
    } else {
        status = sensor_bme280_init(&happ->peripherals.hbme, happ->config.bme280_intf);
        if (status != APP_OK) {
            happ->health.bme280 = APP_COMPONENT_ERROR;
        }

        status = sensor_ds18b20_init(&happ->peripherals.hds, happ->config.how);
        if (status != APP_OK) {
            happ->health.ds18b20 = APP_COMPONENT_ERROR;
        }
    }
}

static App_Status_t sensor_systim_init(TIM_HandleTypeDef *htim) {
    HAL_StatusTypeDef status;

    status = Systim_Init(htim);
    if (status != HAL_OK) {
        return APP_INIT_ERROR;
    }

    return APP_OK;
}

static App_Status_t sensor_bme280_init(BME280_Handle_t *hbme, const BME280_Intf_t *intf) {
    BME280_Status_t status;

    BME280_Config_t config = {0};
    config.osr_press = BME280_OVERSAMPLING_1X;
    config.osr_hum = BME280_OVERSAMPLING_1X;
    config.osr_temp = BME280_OVERSAMPLING_1X;
    config.filter = BME280_FILTER_COEFF_OFF;

    status = BME280_Init(hbme, &config, intf);
    if (status != BME280_STATUS_OK) {
        return APP_INIT_ERROR;
    }

    return APP_OK;
}

static App_Status_t sensor_ds18b20_init(DS18B20_Handle_t *hds, OneWire_Handle_t *how) {
    DS18B20_Status_t status;

    status = DS18B20_Init(hds, how);
    if (status != DS18B20_STATUS_OK) {
        return APP_INIT_ERROR;
    }

    return APP_OK;
}

static App_Status_t ui_init(App_Handle_t *happ) {
    UC8253_Handle_t huc8253 = {0};
    huc8253.busy = happ->config.epd_pins->busy;
    huc8253.cs = happ->config.epd_pins->cs;
    huc8253.dc = happ->config.epd_pins->dc;
    huc8253.res = happ->config.epd_pins->res;
    huc8253.hspi = happ->config.hspi;
    huc8253.height = EPD_HEIGHT;
    huc8253.width = EPD_WIDTH;

    EPD_Config_t epd_config = {0};
    epd_config.frame_buffer = happ->config.retained->frame_buffer;
    epd_config.gfx_buffer = happ->config.gfx_buffer;
    epd_config.frame_size = EPD_SIZE;
    epd_config.hdrv = huc8253;
    epd_config.display_rotation = GFX_ROTATION_270;
    epd_config.skip_frame_buf_clr = happ->config.is_retained_valid;

    happ->peripherals.hui.hepd = &happ->peripherals.hepd;

    if (UI_Ctrl_Init(&happ->peripherals.hui, &epd_config) != UI_CTRL_OK) {
        return APP_INIT_ERROR;
    }

    return APP_OK;
}

App_Status_t systim_run(App_Handle_t *happ) {
    HAL_StatusTypeDef status = HAL_OK;

    if (happ->health.systim == APP_COMPONENT_OK) {
        status = Systim_Start();
    } else {
        return APP_MODULE_FAIL;
    }

    if (status != HAL_OK) {
        happ->health.systim = APP_COMPONENT_ERROR;
        happ->health.ds18b20 = APP_COMPONENT_ERROR;
        happ->health.bme280 = APP_COMPONENT_ERROR;
    }

    return status == HAL_OK ? APP_OK : APP_MODULE_FAIL ;
}

App_Status_t ds18b20_run(App_Handle_t *happ) {
    DS18B20_Status_t status = DS18B20_STATUS_OK;

    if (happ->health.ds18b20 == APP_COMPONENT_OK) {
        status = DS18B20_StartMeas_Async(&happ->peripherals.hds);
    } else {
        return APP_MODULE_FAIL;
    }

    if (status != DS18B20_STATUS_OK) {
        happ->health.ds18b20 = APP_COMPONENT_ERROR;
    }

    return status == DS18B20_STATUS_OK ? APP_OK : APP_MODULE_FAIL;
}

App_Status_t bme280_run(App_Handle_t *happ) {
    BME280_Status_t status = BME280_STATUS_OK;

    if (happ->health.bme280 == APP_COMPONENT_OK) {
        status = BME280_StartMeas_Async(&happ->peripherals.hbme);
    }

    if (status != BME280_STATUS_OK) {
        happ->health.bme280 = APP_COMPONENT_ERROR;
    }

    return status == BME280_STATUS_OK ? APP_OK : APP_MODULE_FAIL;
}

static void handle_measure_phase(App_Handle_t *happ) {
    const bool ds18b20_healthy = (happ->health.ds18b20 == APP_COMPONENT_OK);
    const bool bme280_healthy = (happ->health.bme280 == APP_COMPONENT_OK);

    if (ds18b20_healthy) DS18B20_Task(&happ->peripherals.hds);
    if (bme280_healthy) BME280_Task(&happ->peripherals.hbme);

    const bool ds18b20_ready = (!ds18b20_healthy) || DS18B20_IsDataReady(&happ->peripherals.hds);
    const bool bme280_ready = (!bme280_healthy) || BME280_IsDataReady(&happ->peripherals.hbme);

    if (DS18B20_HasError(&happ->peripherals.hds)) {
        const DS18B20_Error_t error = DS18B20_GetError(&happ->peripherals.hds);
        // printf("DS18B20 error: %d\n", error);
        DS18B20_ClearError(&happ->peripherals.hds);

        happ->health.ds18b20 = APP_COMPONENT_ERROR;
    } else if (BME280_HasError(&happ->peripherals.hbme)) {
        const BME280_Error_t error = BME280_GetError(&happ->peripherals.hbme);
        // printf("BME280 error: %d\n", error);
        BME280_ClearError(&happ->peripherals.hbme);

        happ->health.bme280 = APP_COMPONENT_ERROR;
    } else if (ds18b20_ready && bme280_ready) {
        if (ds18b20_healthy) {
            DS18B20_GetData(&happ->peripherals.hds, &happ->data.ds_temp);
        }

        if (bme280_healthy) {
            BME280_GetData(&happ->peripherals.hbme, &happ->data.bme_data);
        }

        happ->current_phase = APP_PHASE_RENDER;
    }
}

static void handle_render_phase(App_Handle_t *happ) {
    if (happ->health.epd != APP_COMPONENT_OK) {
        happ->current_phase = APP_PHASE_ERROR;
    }

    const float temp_in = (float) happ->data.bme_data.temperature;
    const float *p_temp_in = (happ->health.bme280 == APP_COMPONENT_OK) ? &temp_in : NULL;

    const float hum_in = (float) happ->data.bme_data.humidity;
    const float *p_hum_in  = (happ->health.bme280 == APP_COMPONENT_OK) ? &hum_in : NULL;

    const uint32_t pres_in = (uint32_t) happ->data.bme_data.pressure;
    const uint32_t *p_pres_in = (happ->health.bme280 == APP_COMPONENT_OK) ? (uint32_t*) &pres_in: NULL;

    const float temp_out = happ->data.ds_temp;
    const float *p_temp_out = (happ->health.ds18b20 == APP_COMPONENT_OK) ? &temp_out : NULL;

    UI_Ctrl_DisplayStaticElements(&happ->peripherals.hui);
    UI_Ctrl_DisplayValues(&happ->peripherals.hui, p_temp_in, p_hum_in, p_pres_in, p_temp_out);

    UI_Ctrl_Status_t status;
    if (happ->config.retained->counter % 3 == 0 || !happ->config.is_retained_valid) {
        status = UI_Ctrl_Update(&happ->peripherals.hui);
    } else {
        status = UI_Ctrl_UpdatePartial(&happ->peripherals.hui);
    }

    happ->config.retained->counter++;

    if (status != UI_CTRL_OK) {
        happ->current_phase = APP_PHASE_ERROR;
    }

    happ->current_phase = APP_PHASE_SLEEP;
}

static void handle_sleep_phase() {
    PwrManager_EnterStandby();
}

static void handle_error_phase() {
    BSP_LED_On(LED_GREEN);
}

static void handle_default_phase(App_Handle_t *happ) {
    happ->current_phase = APP_PHASE_ERROR;
}