/**
 * @file onewire_async.h
 *
 * Created on 7/22/26.
 * @author Marek Godlowicz https://github.com/MarekGodlo
 * @brief Provides the public API for controlling a One-Wire bus by bit-banging.
 */

#ifndef WEATHER_STATION_ONEWIRE_ASYNC_H
#define WEATHER_STATION_ONEWIRE_ASYNC_H
#include "onewire_defs.h"
#include "main.h"

/**
 * @brief Represents the One-Wire bus configuration.
 */
typedef struct {
    OneWire_RomMode_t bus_mode; /**< ROM command mode. */
    GPIO_Pin_t *data_pin;       /**< One-Wire data line pin. */ 
    TIM_HandleTypeDef *htim;    /**< Timer used for One-Wire timing. */
} OneWire_Config_t;

/**
 * @brief Represents the One-Wire controller handler.
 */
typedef struct {
    OneWire_Config_t config;               /**< One-Wire hardware configuration. */
    OneWire_Context_t ctx;                 /**< Internal state machine context. */

    uint8_t current_byte;                  /**< Current byte being processed. */
    uint8_t bit_index;                     /**< Current bit index within the byte. */
    uint8_t buf_index;                     /**< Current buffer index. */

    uint8_t *tx_buffer;                    /**< Transmit data buffer. */
    uint8_t tx_len;                        /**< Number of bytes to transmit. */

    uint8_t *rx_buffer;                    /**< Receive data buffer. */
    uint8_t rx_len;                        /**< Number of bytes to receive. */

    volatile OneWire_OpStatus_t op_status; /**< Current operation status. */
} OneWire_Handle_t;

/**
 * @brief Initializes the One-Wire handle.
 *
 * Copies the provided configuration into the handle and resets the
 * interval driver state its initial values.
 *
 * @param how    Pointer to the One-Wire handle.
 * @param config Pointer to the One-Wire configuration.
 *
 * @return Status of the operation.
 * @retval OW_STATUS_OK       Initialization completed successfully.
 * @retval OW_STATUS_NULL_ARG Null pointer passed as argument.
 */
OneWire_Status_t OneWire_Init(OneWire_Handle_t *how, const OneWire_Config_t *config);

/**
 * @brief Starts the One-Wire reset sequence.
 *
 * Configures the internal state machine for a reset operation and
 * immediately invokes the state machine handler.
 *
 * @param how Pointer to the One-Wire handle.
 *
 * @return Status of the operation.
 * @retval OW_STATUS_OK       Reset sequency started successfully.
 * @retval OW_STATUS_BUSY     Another operation is already in progress.
 * @retval OW_STATUS_NULL_ARG Null pointer passed as argument.
 */
OneWire_Status_t OneWire_Master_Reset(OneWire_Handle_t *how);

/**
 * @brief Starts the One-Wire data transmission.
 *
 * Configures the internal state machine for a transmit operation and
 * immediately invokes the state machine handler.
 *
 * @param how  Pointer to the One-Wire handle.
 * @param data Pointer to the transmit data buffer.
 * @param len  Number of bytes to transmit.
 *
 * @return Status of the operation
 * @retval OW_STATUS_OK       Transmission started successfully.
 * @retval OW_STATUS_BUSY     Another operation is already in progress.
 * @retval OW_STATUS_NULL_ARG Null pointer passed as argument.
 */
OneWire_Status_t OneWire_Master_Transmit(OneWire_Handle_t *how, const uint8_t *data, uint8_t len);

/**
 * @brief Starts the One-Wire data reception.
 *
 * Configures the internal state machine for a receive operation and
 * immediately invokes the state machine handler.
 *
 * @param how  Pointer to the One-Wire handle.
 * @param data Pointer to the receive data buffer.
 * @param len  Number of bytes to receive.
 *
 * @return Status of the operation
 * @retval OW_STATUS_OK       Reception started successfully.
 * @retval OW_STATUS_BUSY     Another operation is already in progress.
 * @retval OW_STATUS_NULL_ARG Null pointer passed as argument.
 */
OneWire_Status_t OneWire_Master_Receive(OneWire_Handle_t *how, uint8_t *data, uint8_t len);

/**
 * @brief Returns the las One-Wire error.
 *
 * @param how Pointer to the One-Wire handle.
 *
 * @return Last error status.
 */
OneWire_Error_t OneWire_GetError(OneWire_Handle_t *how);

/**
 * @brief Returns the current One-Wire state.
 *
 * @param how Pointer to the One-Wire handle.
 *
 * @return Current state machine state.
 */
OneWire_State_t OneWire_GetState(OneWire_Handle_t *how);

/**
 * @brief Returns the last One_Wire operation status.
 *
 * @param how Pointer to the One-Wire handle.
 *
 * @return Last One-Wire operation status.
 */
OneWire_OpStatus_t OneWire_GetOperationStatus(OneWire_Handle_t *how);

/**
 * @brief Clears the last One-Wire error.
 *
 * @param how Pointer to the One-Wire handle.
 */
void OneWire_ClearError(OneWire_Handle_t *how);

/**
 * @brief Handles the One-Wire timer completion event.
 *
 * This function is called from the timer interrupt service routine when
 * the configured timer period expires. It triggers the One-Wire state
 * machine execution.
 *
 * @param how Pointer to the One-Wire handle.
 */
void OneWire_OnTimerCplt(OneWire_Handle_t *how);

#endif //WEATHER_STATION_ONEWIRE_ASYNC_H
