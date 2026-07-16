/**
* @file systim.h
*
* Created on 14.07.2026.
* @author Marek Godlowicz https://github.com/MarekGodlo
* @brief
*/

#ifndef WEATHER_STATION_SYSTIM_H
#define WEATHER_STATION_SYSTIM_H
#include "main.h"

HAL_StatusTypeDef Systim_Init(TIM_HandleTypeDef *htim);

HAL_StatusTypeDef Systim_Start(void);
HAL_StatusTypeDef Systim_Stop(void);
uint32_t Systim_GetUs(void);

#endif //WEATHER_STATION_SYSTIM_H
