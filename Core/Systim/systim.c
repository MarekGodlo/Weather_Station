/**
* @file systim.c
*
* Created on 14.07.2026.
* @author Marek Godlowicz https://github.com/MarekGodlo
* @brief
*/

#include "systim.h"

static TIM_HandleTypeDef *sys_htim;

HAL_StatusTypeDef Systim_Init(TIM_HandleTypeDef *htim) {
    if (htim == NULL) return HAL_ERROR;

    sys_htim = htim;
    return HAL_OK;
}

HAL_StatusTypeDef Systim_Start(void) {
    if (sys_htim == NULL) return HAL_ERROR;
    return HAL_TIM_Base_Start(sys_htim);
}

HAL_StatusTypeDef Systim_Stop(void) {
    if (sys_htim == NULL) return HAL_ERROR;
    return HAL_TIM_Base_Stop(sys_htim);
}

uint32_t Systim_GetUs(void) {
    if (sys_htim == NULL) return 0;
    return __HAL_TIM_GET_COUNTER(sys_htim);
}