/**
 * @file systim.h
 *
 * Created on 14.07.2026.
 * @author Marek Godlowicz https://github.com/MarekGodlo
 * @brief System timer utility for microsecond precision timing.
 */

#ifndef WEATHER_STATION_SYSTIM_H
#define WEATHER_STATION_SYSTIM_H
#include "main.h"

/**
 * @brief Initializes the system timer.
 *
 * @param htim Pointer to the hardware timer handle (TIM_HandleTypeDef).
 *
 * @return HAL status of the operation.
 */
HAL_StatusTypeDef Systim_Init(TIM_HandleTypeDef *htim);

/**
 * @brief Starts the system timer.
 *
 * @return HAL status of the operation.
 */
HAL_StatusTypeDef Systim_Start(void);

/**
 * @brief Stops the system timer.
 *
 * @return HAL status of the operation.
 */
HAL_StatusTypeDef Systim_Stop(void);

/**
 * @brief Gets the current time in microseconds.
 *
 * @return Current timer counter value (microseconds).
 */
uint32_t Systim_GetUs(void);

#endif //WEATHER_STATION_SYSTIM_H
