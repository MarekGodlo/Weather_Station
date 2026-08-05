/**
 * @file ds18b20.h
 *
 * Created on 7/17/26.
 * @author Marek Godlowicz https://github.com/MarekGodlo
 * @brief Driver for the DS18B20 digital temperature sensor over One-Wire.
 */

#ifndef WEATHER_STATION_DS18B20_H
#define WEATHER_STATION_DS18B20_H

#include "../OneWire/onewire.h"

/**
 * @brief DS18B20 memory command codes.
 */
#define DS18B20_WRITE_SCRATCHPAD 0x4E
#define DS18B20_READ_SCRATCHPAD 0xBE
#define DS18B20_COPY_SCRATCHPAD 0x48
#define DS18B20_CONVERT_T 0x44
#define DS18B20_RECALL_E2 0xB8
#define DS18B20_READ_POWER_SUPPLY 0xB4

/**
 * @brief Conversion delay values for each measurement resolution, in microseconds.
 */
#define DS18B20_12BIT_DELAY 750000
#define DS18B20_11BIT_DELAY 375000
#define DS18B20_10BIT_DELAY 188000
#define DS18B20_9BIT_DELAY  94000

/**
 * @brief Represents the status codes returned by DS18B20 driver functions.
 */
typedef enum {
    DS18B20_STATUS_OK = 0,  /**< Operation completed successfully. */
    DS18B20_STATUS_BUSY,    /**< Another operation is already in progress. */
    DS18B20_STATUS_NULL_ARG /**< A null pointer was passed as argument. */
} DS18B20_Status_t;

/**
 * @brief Represents the error codes recorded by the internal state machine.
 */
typedef enum {
    DS18B20_ERROR_NONE = 0,  /**< No error has occurred. */
    DS18B20_ERROR_NO_DEVICE, /**< No device was detected on the One-Wire bus. */
    DS18B20_ERROR_CRC,       /**< CRC check of the scratchpad data failed. */
    DS18B20_ERROR_COMM_FAIL, /**< Communication with the device failed. */
    DS18B20_ERROR_OTHER      /**< An unspecified error occurred. */
} DS18B20_Error_t;

/**
 * @brief Represents the phases of the DS18B20 internal state machine.
 */
typedef enum {
    DS18B20_PHASE_IDLE = 0,        /**< No operation in progress. */
    DS18B20_PHASE_RESET_SEQ,       /**< Performing the One-Wire reset sequence. */
    DS18B20_PHASE_START_MEAS,      /**< Issuing the start-measurement command. */
    DS18B20_PHASE_WAIT,            /**< Waiting for the conversion delay to elapse. */
    DS18B20_PHASE_REQ_SCRATCHPAD,  /**< Requesting the scratchpad data. */
    DS18B20_PHASE_READ_SCRATCHPAD, /**< Reading the scratchpad data. */
    DS18B20_PHASE_DONE,            /**< Measurement data is ready to be read. */
    DS18B20_PHASE_ERROR,           /**< An error has occurred. */
} DS18B20_Phase_t;

/**
 * @brief Represents the internal state machine context of the DS18B20 driver.
 */
typedef struct {
    DS18B20_Phase_t phase;      /**< Current phase of the state machine. */
    DS18B20_Phase_t next_phase; /**< Phase to transition to after the current one completes. */

    bool is_busy;    /**< True if an operation is currently in progress. */
    bool op_started; /**< True if the current phase's operation has been started. */

    DS18B20_Error_t last_error; /**< Last error recorded by the state machine. */

    uint32_t deadline_us; /**< Required delay before the measurement is ready, in microseconds. */
    uint32_t now_us;      /**< Timestamp taken when the measurement was started, in microseconds. */
} DS18B20_Contex_t;

/**
 * @brief Represents the DS18B20 sensor handle.
 */
typedef struct {
    DS18B20_Contex_t ctx;  /**< Internal state machine context. */
    OneWire_Handle_t *how; /**< Pointer to the associated One-Wire handle. */

    uint8_t cmd_buf;       /**< Buffer holding the command byte sent over One-Wire. */
    uint8_t scratchpad[9]; /**< Raw scratchpad data read from the device. */
} DS18B20_Handle_t;

/**
 * @brief Initializes the DS18B20 handle.
 *
 * Attaches the One-Wire handle by reference and resets the
 * internal driver state to its initial values.
 *
 * @param hds Pointer to the DS18B20 handle.
 * @param how Pointer to the One-Wire handle.
 *
 * @return Status of the operation.
 * @retval DS18B20_STATUS_OK       Initialization completed successfully.
 * @retval DS18B20_STATUS_NULL_ARG Null pointer passed as argument.
 */
DS18B20_Status_t DS18B20_Init(DS18B20_Handle_t *hds, OneWire_Handle_t *how);

/**
 * @brief Starts an asynchronous measurement operation.
 *
 * This function is non-blocking; progress must be advanced by calling
 * DS18B20_Task().
 *
 * @param hds Pointer to the DS18B20 handle.
 *
 * @return Status of the operation.
 * @retval DS18B20_STATUS_OK       Measurement start initiated successfully.
 * @retval DS18B20_STATUS_BUSY     Another operation is already in progress.
 * @retval DS18B20_STATUS_NULL_ARG Null pointer passed as argument.
 */
DS18B20_Status_t DS18B20_StartMeas_Async(DS18B20_Handle_t *hds);

/**
 * @brief Checks whether the measurement data is ready to be read.
 *
 * @param hds Pointer to the DS18B20 handle.
 *
 * @return True if the data is ready to be read, false otherwise
 *         (including when hds is NULL).
 */
bool DS18B20_IsDataReady(const DS18B20_Handle_t *hds);

/**
 * @brief Retrieves the converted temperature from the last measurement.
 *
 * Reads and converts the raw scratchpad data into a temperature value,
 * then resets the internal state machine to idle, allowing a new
 * measurement to be started.
 *
 * @param hds  Pointer to the DS18B20 handle.
 * @param temp Pointer to the variable that will receive the temperature
 *             value, in degrees Celsius.
 *
 * @return Status of the operation.
 * @retval DS18B20_STATUS_OK       Data was retrieved successfully.
 * @retval DS18B20_STATUS_NULL_ARG Null pointer passed as argument.
 *
 * @note The caller should verify that data is ready via
 *       DS18B20_IsDataReady() before calling this function.
 */
DS18B20_Status_t DS18B20_GetData(DS18B20_Handle_t *hds, float *temp);

/**
 * @brief Checks if the internal state machine has an error.
 *
 * @param hds Pointer to the DS18B20 handle.
 * @return True if an error is present, false otherwise
 *         (including when hds is NULL).
 */
bool DS18B20_HasError(const DS18B20_Handle_t *hds);

/**
 * @brief Returns the last error recorded by the internal state machine.
 *
 * @param hds Pointer to the DS18B20 handle.
 *
 * @return The last recorded error, including DS18B20_ERROR_NONE if no error occurred.
 */
DS18B20_Error_t DS18B20_GetError(const DS18B20_Handle_t *hds);

/**
 * @brief Clears the error state and resets the internal state machine to idle.
 *
 * @param hds Pointer to the DS18B20 handle.
 *
 * @return Status of the operation.
 * @retval DS18B20_STATUS_OK       Operation completed successfully.
 * @retval DS18B20_STATUS_NULL_ARG Null pointer passed as argument.
 */
DS18B20_Status_t DS18B20_ClearError(DS18B20_Handle_t *hds);

/**
 * @brief Manages the state machine; controls the measurement process.
 *
 * @param hds Pointer to the Ds18B20 handle.
 */
void DS18B20_Task(DS18B20_Handle_t *hds);


#endif //WEATHER_STATION_DS18B20_H
