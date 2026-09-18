/**
 * @file pwr_manager.h
 *
 * Created on 9/18/26.
 * @author Marek Godlowicz https://github.com/MarekGodlo
 * @brief Manages power modes, RTC-based wake-up, and data retention in RAM2.
 */

#ifndef WEATHER_STATION_PWR_MANAGER_H
#define WEATHER_STATION_PWR_MANAGER_H
#include <stdbool.h>
#include "main.h"
#include "../EPD/epd.h"

/**
 * @brief Represents the power manager handle.
 */
typedef struct {
    bool rtc_init_flag;      /**< True if RTC has been initialized. */
    bool standby_flag;       /**< True if MCU woke up from STANDBY mode. */
    bool alarm_a_flag;       /**< True if MCU woke up by Alarm A. */
    bool wu_tim_flag;        /**< True if MCU woke up by the Wake-up Timer. */
    bool is_retained_valid;  /**< True if the retained data is valid. */
    RTC_HandleTypeDef *hrtc; /** Pointer to the RTC handle. */
} PwrManager_Handle_t;

/**
 * @brief Holds data retained in RAM2.
 */
typedef struct {
    uint32_t magic;                 /**< Magic value used to validate the retained data. */
    uint8_t frame_buffer[EPD_SIZE]; /**< Buffer storing the display frame for retention. */
    uint8_t counter;                /**< Counter for display refresh logic. */
} PwrManager_Retained_t;

/**
 * @brief Initializes the power manager handle and sets wake-up flags.
 *
 * @param hpwr Pointer to the power manager handle.
 * @param hrtc Pointer to the RTC handle.
 *
 * @return Status of the operation.
 * @retval HAL_OK Initialization completed successfully.
 */
HAL_StatusTypeDef PwrManager_Init(PwrManager_Handle_t *hpwr, RTC_HandleTypeDef *hrtc);

/**
 * @brief Retrieves the pointer to the retained data structure in RAM2.
 *
 * @return Pointer to the PwrManager_Retained_t structure.
 */
PwrManager_Retained_t* PwrManager_GetRetained(void);

/**
 * @brief Checks if the retained data is valid based on magic value and wake-up source.
 *
 * @param hpwr Pointer to the power manager handle.
 *
 * @return True if the retained data is valid, false otherwise.
 */
bool PwrManager_CheckRetained(const PwrManager_Handle_t *hpwr);

/**
 * @brief Checks if the system woke up from Standby mode.
 *
 * @param hpwr Pointer to the power manager handle.
 *
 * @return True if woke up from standby, false otherwise.
 */
bool PwrManager_WasStandby(const PwrManager_Handle_t *hpwr);

/**
 * @brief Checks if the RTC has been previously initialized.
 *
 * @param hpwr Pointer to the power manager handle.
 *
 * @return True if RTC was initialized, false otherwise.
 */
bool PwrManager_WasRtcInit(const PwrManager_Handle_t *hpwr);

/**
 * @brief Sets the RTC initialization flag in the backup domain.
 *
 * @param hpwr Pointer to the power manager handle.
 */
void PwrManager_SetRtcInitFlag(const PwrManager_Handle_t *hpwr);

/**
 * @brief Enters the MCU into Standby mode.
 *
 * The MCU will be woken up by RTC Alarm A or Wake-up Timer.
 */
void PwrManager_EnterStandby(void);

#endif //WEATHER_STATION_PWR_MANAGER_H
