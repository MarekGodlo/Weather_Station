/**
 * @file app_logic.h
 *
 * Created on 9/20/26.
 * @author Marek Godlowicz https://github.com/MarekGodlo
 * @brief Provides the core logic and state machine for the weather station application.
 */

#ifndef WEATHER_STATION_APP_LOGIC_H
#define WEATHER_STATION_APP_LOGIC_H
#include "main.h"

#include "../BME280/bme280_ctrl.h"

#include "../DS18B20/ds18b20.h"
#include "../UI/ui_ctrl.h"

#include "../PowerManager/pwr_manager.h"

/**
 * @brief Represents status of an operation
 */
typedef enum {
    APP_OK = 0,     /**< Operation finished successfully. */
    APP_NULL_ARG,   /**< Null pointer was passed as an argument. */
    APP_INIT_ERROR, /**< Initialization failed. */
    APP_MODULE_FAIL /**< A module reported a failure during operation. */
} App_Status_t;

/**
 * @brief Represents the status of each component.
 */
typedef enum {
    APP_COMPONENT_OK = 0, /**< The component is healthy. */
    APP_COMPONENT_ERROR,  /**< The component is faulty. */
} App_ComponentStatus_t;

/**
 * @brief Holds the current status of each system component,
 *        allowing the system to keep running even if one component fails.
 */
typedef struct {
    App_ComponentStatus_t bme280;  /**< Status of the BME280 sensor module. */
    App_ComponentStatus_t ds18b20; /**< Status of the DS18B20 sensor module. */
    App_ComponentStatus_t epd;     /**< Status of the e-paper display module. */
    App_ComponentStatus_t systim;  /**< Status of the systim module. */
} App_Health_t;

/**
 * @brief Represents phase of the system state machine. 
 */
typedef enum {
    APP_PHASE_INIT,    /**< System initialization in progress. */
    APP_PHASE_MEASURE, /**< Sensor measurement in progress. */
    APP_PHASE_RENDER,  /**< Rendering and updating the display. */
    APP_PHASE_SLEEP,   /**< System in low-power sleep mode. */
    APP_PHASE_ERROR,   /**< System reached an unrecoverable error state. */
} App_Phase_t;

/**
 * @brief Represents the configuration parameters for the application.
 */
typedef struct {
    TIM_HandleTypeDef *systim_tim;   /**< Pointer to the timer handle used by the systim module. */

    const EPD_Pins_t *epd_pins;      /**< Pointer to the EPD pins configuration. */

    BME280_Intf_t *bme280_intf;      /**< Pointer to the BME280 interface structure. */
    SPI_HandleTypeDef *hspi;         /**< Pointer to the SPI handle used for EPD communication. */

    PwrManager_Handle_t *hpwr;       /**< Pointer to the power manager handle. */
    OneWire_Handle_t *how;           /**< Pointer to the One-Wire handle. */

    PwrManager_Retained_t *retained; /**< Pointer to the retained memory structure. */
    bool is_retained_valid;          /**< True if the retained data is valid. */
    uint8_t *gfx_buffer;             /**< Pointer to the graphics buffer for the display. */
} App_Config_t;

/**
 * @brief Holds handles for all system peripherals.
 */
typedef struct {
    BME280_Handle_t hbme;  /**< BME280 sensor handle. */
    DS18B20_Handle_t hds;  /**< DS18B20 sensor handle. */
    UI_Ctrl_Handle_t hui;  /**< UI controller handle. */
    EPD_Handle_t hepd;     /**< EPD driver handle. */
} App_Peripherals_t;

/**
 * @brief Holds the latest data read from the sensors.
 */
typedef struct {
    BME280_Data_t bme_data; /**< Data from the BME280 sensor (temp, hum, press). */
    float ds_temp;          /**< Temperature from the DS18B20 sensor. */
} App_SensorData_t;

/**
 * @brief Represents the main application handle.
 */
typedef struct {
    App_Phase_t current_phase;      /**< Current phase of the system state machine. */
    App_Peripherals_t peripherals; /**< System peripheral handles. */
    App_Config_t config;           /**< Application configuration. */
    App_Health_t health;           /**< Health status of system components. */
    App_SensorData_t data;         /**< Latest sensor measurement data. */
} App_Handle_t;

/**
 * @brief Initializes the application logic and all sub-modules.
 *
 * @param happ   Pointer to the application handle.
 * @param config Pointer to the configuration structure.
 *
 * @return Status of the operation.
 * @retval APP_OK         Initialization completed successfully.
 * @retval APP_NULL_ARG   Null pointer passed as argument.
 * @retval APP_INIT_ERROR Initialization of a sub-module failed.
 */
App_Status_t App_Init(App_Handle_t *happ, const App_Config_t *config);

/**
 * @brief Runs the main application logic.
 *
 * @param happ Pointer to the application handle.
 *
 * @return Status of the operation.
 */
App_Status_t App_Run(App_Handle_t *happ);

/**
 * @brief Main application task; manages the system state machine.
 *
 * This function should be called periodically in the main loop.
 *
 * @param happ Pointer to the application handle.
 */
void App_Task(App_Handle_t *happ);
#endif //WEATHER_STATION_APP_LOGIC_H
