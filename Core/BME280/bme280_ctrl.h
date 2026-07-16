/**
 * @file bme280_ctrl.h
 *
 * Created on 14.07.2026.
 * @author Marek Godlowicz https://github.com/MarekGodlo
 * @brief Non-blocking driver for the BME280 sensor, built on top of the
 *       Bosch BME280 API, exposing a polling-based state machine interface.
 */

#ifndef WEATHER_STATION_BME280_CTRL_H
#define WEATHER_STATION_BME280_CTRL_H
#include "BME_Driver/bme280_defs.h"
#include <stdbool.h>
#include "main.h"

/**
 * @brief Represents the internal phases of the BME280 measurement state machine.
 */
typedef enum {
    BME280_PHASE_IDLE = 0,          /**< No measurement in progress; waiting to be triggered. */
    BME280_PHASE_START_MEASUREMENT, /**< Measurement is being triggered on the sensor. */
    BME280_PHASE_WAIT,              /**< Waiting for the sensor to finish measuring. */
    BME280_PHASE_READ_DATA,         /**< Reading the measured data from the sensor. */
    BME280_PHASE_DONE,              /**< Data is ready and waiting to be retrieved. */
    BME280_PHASE_ERROR              /**< An error has occurred; must be cleared before continuing. */
} BME280_Phase_t;

/**
 * @brief Represents the status of the BME280 driver's operations.
 */
typedef enum {
    BME280_STATUS_OK = 0,  /**< Successful. */
    BME280_STATUS_BUSY,    /**< A measurement is in progress or unread data is pending. */
    BME280_STATUS_ERROR,   /**< Operation failed. */
    BME280_STATUS_NULL_ARG /**< A required argument was NULL. */
} BME280_Status_t;

/**
 * @brief Represents the error codes recorded by the BME280 internal state machine.
 */
typedef enum {
    BME280_ERROR_NONE = 0,       /**< No error has occurred. */
    BME280_ERROR_COMM_FAIL,      /**< Communication with the device failed. */
    BME280_ERROR_INVALID_CONFIG, /**< The requested configuration could not be applied. */
    BME280_ERROR_PARSE_FAIL,     /**< Failed to parse or compensate the measurement data. */
    BME280_ERROR_INVALID_PHASE,  /**< Operation requested is not valid in the current phase. */
    BME280_ERROR_OTHER           /**< An unspecified error occurred. */
} BME280_Error_t;

/**
 * @brief Represents the compensated sensor measurement data.
 */
typedef struct {
    double pressure;    /**< Compensated pressure. */
    double temperature; /**< Compensated temperature. */
    double humidity;    /**< Compensated humidity. */
} BME280_Data_t;

/**
 * @brief Represents the hardware interface used to communicate with the sensor.
 */
typedef struct {
    I2C_HandleTypeDef *hi2c; /**< Pointer to the I2C handler. */
    uint8_t i2c_addr;        /**< I2C address of the sensor. */
    TIM_HandleTypeDef *htim; /**< Pointer to the Timer handler, used for delays. */
} BME280_Intf_t;

/**
 * @brief Represents the user-configurable measurement settings.
 */
typedef struct {
    uint8_t osr_temp;  /**< Oversampling value for temperature. */
    uint8_t osr_press; /**< Oversampling value for pressure. */
    uint8_t osr_hum;   /**< Oversampling value for humidity. */
    uint8_t filter;    /**< IIR filter coefficient. */
    bool dirty;        /**< True if the settings have not yet been synced to the sensor. */
} BME280_Config_t;

/**
 * @brief Represents the internal state machine context.
 */
typedef struct {
    BME280_Phase_t phase; /**< Current internal phase of the measurement cycle. */

    bool is_busy;              /**< True if a measurement is currently in progress. */
    BME280_Error_t last_error; /**< Last error recorded by the state machine. */

    uint32_t deadline_us; /**< Required delay before the measurement is ready, in microseconds. */
    uint32_t now_us;      /**< Timestamp taken when the measurement was started, in microseconds. */
} BME280_Contex_t;

/**
 * @brief Represents the BME280 sensor handler.
 */
typedef struct {
    struct bme280_dev dev;  /**< Bosch BME280 driver device structure. */
    BME280_Intf_t intf;     /**< Hardware interface configuration. */
    BME280_Contex_t ctx;    /**< Internal state machine context. */
    BME280_Data_t data;     /**< Last compensated measurement data. */
    BME280_Config_t config; /**< User-configurable measurement settings. */
} BME280_Handle_t;

/**
 * @brief Initializes the BME280 handler by setting default values and configuring the sensor.
 *
 * @param hbme Pointer to the BME280 handler.
 * @param config Pointer to the measurement configuration to apply.
 * @param intf   Pointer to the hardware interface configuration.
 *
 * @return BME280_STATUS_OK on success, error status otherwise.
 */
BME280_Status_t BME280_Init(BME280_Handle_t *hbme, const BME280_Config_t *config, const BME280_Intf_t *intf);

/**
 * @brief Starts a measurement by triggering the state machine.
 *
 * @param hbme Pointer to the BME280 handler.
 * @retval BME280_STATUS_OK       Measurement started successfully.
 * @retval BME280_STATUS_BUSY     A measurement is already in progress or unread data is pending.
 * @retval BME280_STATUS_NULL_PTR hbme is NULL.
 */
BME280_Status_t BME280_StartMeas_Async(BME280_Handle_t *hbme);

/**
 * @brief Checks if the sensor is busy.
 *
 * @param hbme Pointer to the BME280 handler.
 * @retval true  The sensor is currently busy.
 * @retval false The sensor is not busy, or the param is NULL.
 */
bool BME280_IsBusy(const BME280_Handle_t *hbme);

/**
 * @brief Checks if sensor data is available.
 *
 * @param hbme Pointer to the BME280 handler.
 * @retval true  Sensor data is ready.
 * @retval false Sensor data is not ready yet, or hbme is NULL.
 */
bool BME280_IsDataReady(const BME280_Handle_t *hbme);

/**
 * @brief Gets the sensor data from the BME280 handler.
 *
 * @param hbme Pointer to the BME280 handler.
 * @param data Pointer to the struct that represents sensor data.
 * @retval BME280_STATUS_OK       Data copied successfully.
 * @retval BME280_STATUS_BUSY     Data is not ready yet; measurement still in progress.
 * @retval BME280_STATUS_NULL_PTR hbme or data is NULL.
 */
BME280_Status_t BME280_GetData(BME280_Handle_t *hbme, BME280_Data_t *data);

/**
 * @brief Checks whether the internal state machine has an error.
 *
 * @param hbme Pointer to the BME280 handle.
 *
 * @return True if an error is present, false otherwise
 *         (including when hbme is NULL).
 */
bool BME280_HasError(const BME280_Handle_t *hbme);

/**
 * @brief Returns the last error recorded by the internal state machine.
 *
 * @param hbme Pointer to the BME280 handle.
 *
 * @return The last recorded error, or BME280_ERROR_NONE if hbme is NULL
 */
BME280_Error_t BME280_GetError(const BME280_Handle_t *hbme);

/**
 * @brief Clears the error state and resets the internal state machine to idle.
 *
 * @param hbme Pointer to the BME280 handle.
 *
 * @return Status of the operation.
 * @retval BME280_STATUS_OK       Operation completed successfully.
 * @retval BME280_STATUS_NULL_ARG Null pointer passed as argument
 */
BME280_Status_t BME280_ClearError(BME280_Handle_t *hbme);

/**
 * @brief Manages the state machine; controls the measurement process.
 *
 * @param hbme Pointer to the BME280 handler.
 */
void BME280_Task(BME280_Handle_t *hbme);
#endif //WEATHER_STATION_BME280_CTRL_H
