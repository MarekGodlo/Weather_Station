/**
 * @file onewire_defs.h
 *
 * Created on 8/5/26.
 * @author Marek Godlowicz https://github.com/MarekGodlo
 * @brief Defines types and enumerations used by the One-Wire driver.
 */

#ifndef WEATHER_STATION_ONEWIRE_DEFS_H
#define WEATHER_STATION_ONEWIRE_DEFS_H
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief One-Wire ROM command codes.
 */
#define OW_READ_ROM 0x33
#define OW_MATCH_ROM 0x55
#define OW_SKIP_ROM 0xCC
#define OW_SEARCH_ROM 0xF0
#define OW_ALARM_SEARCH 0xEC

/**
 * @brief Represents the status returned by One-Wire API functions.
 */
typedef enum {
    OW_STATUS_OK = 0,  /**< Operation completed successfully. */
    OW_STATUS_BUSY,    /**< Another operation is already in progress. */
    OW_STATUS_NULL_ARG /**< Null pointer passed as argument. */
} OneWire_Status_t;


/**
 * @brief Represents the last One-Wire error condition.
 */
typedef enum {
    OW_ERROR_NONE = 0,         /**< No error occurred. */
    OW_ERROR_NO_DEVICE,        /**< No One-Wire device detected. */
    OW_ERROR_INVALID_MODE,     /**< Invalid ROM operation mode. */
    OW_ERROR_INVALID_STATE,    /**< Invalid state machine state. */
    OW_ERROR_INVALID_SUB_STATE /**< Invalid state machine sub-state. */
} OneWire_Error_t;


/**
 * @brief Represents the One-Wire ROM operation mode.
 */
typedef enum {
    OW_ROM_MODE_SKIP = 0 /**< Skip ROM command mode. */
} OneWire_RomMode_t;


/**
 * @brief Represents the bit-banging operation status.
 */
typedef enum {
    OW_BB_STATUS_OK = 0, /**< Bit-banging operation completed successfully. */
    OW_BB_STATUS_BUSY,   /**< Bit-banging operation is in progress. */
    OW_BB_STATUS_ERROR   /**< Bit-banging operation failed. */
} OneWire_BitBang_Status_t;


/**
 * @brief Represents the One-Wire state machine state.
 */
typedef enum {
    OW_STATE_IDLE = 0, /**< No operation is active. */
    OW_STATE_RESET,    /**< Reset pulse sequence. */
    OW_STATE_ROM,      /**< ROM command processing. */
    OW_STATE_READ,     /**< Data read operation. */
    OW_STATE_WRITE,    /**< Data write operation. */
    OW_STATE_ERROR     /**< Error state. */
} OneWire_State_t;


/**
 * @brief Represents the One-Wire operation status.
 */
typedef enum {
    OW_OPERATION_RESET = 0, /**< Reset operation. */
    OW_OPERATION_BUSY,      /**< Operation is currently running. */
    OW_OPERATION_COMPLETE,  /**< Operation completed successfully. */
    OW_OPERATION_ERROR      /**< Operation failed. */
} OneWire_OpStatus_t;


/**
 * @brief Represents the internal One-Wire state machine context.
 */
typedef struct {
    volatile OneWire_State_t state;                   /**< Current state machine state. */
    volatile uint8_t sub_state;                       /**< Current state machine sub-state. */
    volatile OneWire_BitBang_Status_t bit_bang_state; /**< Current bit-banging status. */

    volatile bool is_busy;                           /**< Indicates whether an operation is active. */
    volatile OneWire_Error_t last_error;             /**< Last occurred One-Wire error. */
} OneWire_Context_t;
#endif //WEATHER_STATION_ONEWIRE_DEFS_H
