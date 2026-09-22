/**
 * @file pwr_manager.c
 *
 * Created on 9/18/26.
 * @author Marek Godlowicz https://github.com/MarekGodlo
 * @brief Manages power modes, RTC-based wake-up, and data retention in RAM2.
 */

#include "pwr_manager.h"

#define RTC_INIT_MAGIC 0xF8F83232
#define RETAINED_MAGIC 0x2D2D2D2D

static bool validate_retained();
static bool validate_backup_data(const RTC_HandleTypeDef *hrtc, uint32_t backup_register, uint32_t expected_value);
static void configure_wakeup_source(void);
static void check_all_flags(PwrManager_Handle_t *hpwr);
static void clear_all_flags(void);

__attribute__((__section__(".retained"))) static PwrManager_Retained_t retained;

HAL_StatusTypeDef PwrManager_Init(PwrManager_Handle_t *hpwr, RTC_HandleTypeDef *hrtc) {
    assert_param(hpwr != NULL);
    assert_param(hrtc != NULL);

    if (hpwr == NULL || hrtc == NULL) {
        return HAL_ERROR;
    }

    hpwr->hrtc = hrtc;
    hpwr->rtc_init_flag = false;
    hpwr->standby_flag = false;
    hpwr->alarm_a_flag = false;
    hpwr->wu_tim_flag = false;
    hpwr->is_retained_valid = false;

    __HAL_RCC_RTCAPB_CLK_ENABLE();

    check_all_flags(hpwr);
    clear_all_flags();

    if (!hpwr->is_retained_valid) {
        retained.magic = RETAINED_MAGIC;
        retained.counter = 0;
    }

    return HAL_OK;
}

PwrManager_Retained_t* PwrManager_GetRetained(void) {
    return &retained;
}

bool PwrManager_CheckRetained(const PwrManager_Handle_t *hpwr) {
    assert_param(hpwr != NULL);
    return hpwr->is_retained_valid;
}

bool PwrManager_WasStandby(const PwrManager_Handle_t *hpwr) {
    assert_param(hpwr != NULL);
    return hpwr->standby_flag;
}

bool PwrManager_WasRtcInit(const PwrManager_Handle_t *hpwr) {
    assert_param(hpwr != NULL);
    return hpwr->rtc_init_flag;
}

void PwrManager_SetRtcInitFlag(const PwrManager_Handle_t *hpwr) {
    assert_param(hpwr != NULL);

    HAL_PWR_EnableBkUpAccess();
    HAL_RTCEx_BKUPWrite(hpwr->hrtc, RTC_BKP_DR0, RTC_INIT_MAGIC);
    HAL_PWR_DisableBkUpAccess();
}

void PwrManager_EnterStandby(void) {
    configure_wakeup_source();
    HAL_PWREx_EnableSRAM2ContentStandbyRetention(PWR_SRAM2_PAGE2_STANDBY_RETENTION);
    HAL_DBGMCU_EnableDBGStandbyMode();
    HAL_PWR_EnterSTANDBYMode();
}

static void check_all_flags(PwrManager_Handle_t *hpwr) {
    hpwr->standby_flag = __HAL_PWR_GET_FLAG(PWR_FLAG_SBF);
    hpwr->alarm_a_flag = __HAL_RTC_GET_FLAG(&hrtc, RTC_FLAG_ALRAF);
    hpwr->wu_tim_flag = __HAL_RTC_GET_FLAG(&hrtc, RTC_FLAG_WUTF);
    hpwr->is_retained_valid = validate_retained();
    hpwr->rtc_init_flag = validate_backup_data(hpwr->hrtc, RTC_BKP_DR0, RTC_INIT_MAGIC);
}

static void clear_all_flags(void) {
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_SBF);
    __HAL_RTC_CLEAR_FLAG(&hrtc, RTC_FLAG_ALRAF);
    __HAL_RTC_CLEAR_FLAG(&hrtc, RTC_FLAG_WUTF);
}

static void configure_wakeup_source(void) {
    __HAL_RCC_PWR_CLK_ENABLE();
    HAL_PWR_EnableWakeUpLine(PWR_WAKEUP_LINE7, PWR_WAKEUP_SELECT_3, PWR_WAKEUP_POLARITY_HIGH);
}

static bool validate_retained() {
    return retained.magic == RETAINED_MAGIC;
}

static bool validate_backup_data(const RTC_HandleTypeDef *hrtc, const uint32_t backup_register, const uint32_t expected_value) {
    bool is_valid = false;

    HAL_PWR_EnableBkUpAccess();
    if (HAL_RTCEx_BKUPRead(hrtc, backup_register) == expected_value) {
        is_valid = true;
    }
    HAL_PWR_DisableBkUpAccess();

    return is_valid;
}