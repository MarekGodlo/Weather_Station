/**
 * @file uc8253.c
 *
 * Created on 19.06.2026.
 * @author Marek Godlowicz https://github.com/MarekGodlo
 * @brief Low-level driver for the UC8253 e-paper display controller.
 */

#include "uc8253.h"

#include <stdbool.h>
#include <string.h>

#define CHECK_STATUS(_expr) do { \
    const UC8253_Status_t _s = (_expr); \
    if (_s != UC8253_OK) { \
        return _s; \
    } \
} while (0) \

static UC8253_Status_t send_img_data(UC8253_Handle_t *hdrv, uint16_t size, const uint8_t *img);

static UC8253_Status_t spi_write_cmd(const UC8253_Handle_t *hdrv, uint8_t cmd);
static UC8253_Status_t spi_write_data(const UC8253_Handle_t *hdrv, uint8_t data);

/** User-provided implementation **/
//@{
static void set_pin(GPIO_Pin_t pin);
static void reset_pin(GPIO_Pin_t pin);
static bool read_pin(GPIO_Pin_t pin);

static UC8253_Status_t spi_write(const UC8253_Handle_t *hdrv, uint8_t data);
static UC8253_Status_t wait_for_busy(const UC8253_Handle_t *hdrv);
static void delay_ms(uint16_t delay);
//}@

UC8253_Status_t UC8253_Init(UC8253_Handle_t *hdrv) {
    UC8253_Reset(hdrv);

    CHECK_STATUS(spi_write_cmd(hdrv, UC8253_PANEL_SETTING));
    CHECK_STATUS(spi_write_data(hdrv, 0xDF));
    CHECK_STATUS(spi_write_data(hdrv, 0x0E));

    CHECK_STATUS(spi_write_cmd(hdrv, UC8253_POWER_ON));
    CHECK_STATUS(wait_for_busy(hdrv));

    CHECK_STATUS(spi_write_cmd(hdrv, UC8253_CDI));

    CHECK_STATUS(spi_write_data(hdrv, 0x97)); // to set white border
    // CHECK_STATUS(spi_write_data(hdrv, 0x57));   // to set black border

    return UC8253_OK;
}

UC8253_Status_t UC8253_Init_Partial(UC8253_Handle_t *hdrv) {
    UC8253_Reset(hdrv);

    CHECK_STATUS(spi_write_cmd(hdrv, UC8253_PANEL_SETTING));
    CHECK_STATUS(spi_write_data(hdrv, 0xDF));
    CHECK_STATUS(spi_write_data(hdrv, 0x0E));

    CHECK_STATUS(spi_write_cmd(hdrv, UC8253_POWER_ON));
    CHECK_STATUS(wait_for_busy(hdrv));

    CHECK_STATUS(spi_write_cmd(hdrv, UC8253_CASCADE_SETTING));
    CHECK_STATUS(spi_write_data(hdrv, 0x02));

    CHECK_STATUS(spi_write_cmd(hdrv, UC8253_FORCE_TEMP));
    CHECK_STATUS(spi_write_data(hdrv, 0x6E));

    CHECK_STATUS(spi_write_cmd(hdrv, UC8253_CDI));
    CHECK_STATUS(spi_write_data(hdrv, 0xD7));

    return UC8253_OK;
}

UC8253_Status_t UC8253_EnterPartialMode(UC8253_Handle_t *hdrv, uint16_t x_start, const uint16_t y_start,
                          const uint16_t width, const uint16_t height) {
    x_start = x_start - x_start % 8;
    const uint16_t x_end = x_start + width - 1;
    const uint16_t y_end = y_start + height - 1;

    CHECK_STATUS(spi_write_cmd(hdrv, UC8253_PARTIAL_IN));

    CHECK_STATUS(spi_write_cmd(hdrv, UC8253_PARTIAL_WINDOW));
    CHECK_STATUS(spi_write_data(hdrv, x_start));
    CHECK_STATUS(spi_write_data(hdrv, x_end));

    CHECK_STATUS(spi_write_data(hdrv, y_start / 256));
    CHECK_STATUS(spi_write_data(hdrv, y_start % 256));

    CHECK_STATUS(spi_write_data(hdrv, y_end / 256));
    CHECK_STATUS(spi_write_data(hdrv, y_end % 256));

    CHECK_STATUS(spi_write_data(hdrv, 0x01));

    return UC8253_OK;
}

UC8253_Status_t UC8253_UpdateDisplay(UC8253_Handle_t *hdrv) {
    CHECK_STATUS(spi_write_cmd(hdrv, UC8253_DISPLAY_REFRESH));
    delay_ms(1); // requires delay greater than 200us
    CHECK_STATUS(wait_for_busy(hdrv));

    return UC8253_OK;
}

UC8253_Status_t UC8253_WriteImgOld(UC8253_Handle_t *hdrv, const uint16_t size, const uint8_t *old_img) {
    CHECK_STATUS(spi_write_cmd(hdrv, UC8253_START_TX1));
    CHECK_STATUS(send_img_data(hdrv, size, old_img));

    return UC8253_OK;
}

UC8253_Status_t  UC8253_WriteImgNew(UC8253_Handle_t *hdrv, const uint16_t size, const uint8_t *new_img) {
    CHECK_STATUS(spi_write_cmd(hdrv, UC8253_START_TX2));
    CHECK_STATUS(send_img_data(hdrv, size, new_img));

    return UC8253_OK;
}

uint16_t UC8253_GetFullFrameSize(UC8253_Handle_t *hdrv) {
    return hdrv->height * (hdrv->width / 8);
}

UC8253_Status_t UC8253_EnterDeepSleep(UC8253_Handle_t *hdrv) {
    CHECK_STATUS(UC8253_PowerOff(hdrv));

    CHECK_STATUS(spi_write_cmd(hdrv, UC8253_DEEP_SLEEP));
    CHECK_STATUS(spi_write_data(hdrv, 0xA5));

    return UC8253_OK;
}

void UC8253_Reset(UC8253_Handle_t *hdrv) {
    reset_pin(hdrv->res);
    delay_ms(5);
    set_pin(hdrv->res);
    delay_ms(10);
}

UC8253_Status_t UC8253_PowerOff(UC8253_Handle_t *hdrv) {
    CHECK_STATUS(spi_write_cmd(hdrv, UC8253_POWER_OFF));
    CHECK_STATUS(wait_for_busy(hdrv));

    return UC8253_OK;
}

UC8253_Status_t UC8253_PartialOut(UC8253_Handle_t *hdrv) {
    CHECK_STATUS(spi_write_cmd(hdrv, UC8253_PARTIAL_OUT));
    return UC8253_OK;
}

static UC8253_Status_t send_img_data(UC8253_Handle_t *hdrv, const uint16_t size, const uint8_t *img) {
    for (int i = 0; i < size; ++i) {
        CHECK_STATUS(spi_write_data(hdrv, img[i]));
    }

    return UC8253_OK;
}

static UC8253_Status_t spi_write_cmd(const UC8253_Handle_t *hdrv, const uint8_t cmd) {
    reset_pin(hdrv->cs);

    reset_pin(hdrv->dc);
    CHECK_STATUS(spi_write(hdrv, cmd));
    set_pin(hdrv->cs);

    return UC8253_OK;
}

static UC8253_Status_t spi_write_data(const UC8253_Handle_t *hdrv, const uint8_t data) {
    reset_pin(hdrv->cs);

    set_pin(hdrv->dc);
    CHECK_STATUS(spi_write(hdrv, data));
    set_pin(hdrv->cs);

    return UC8253_OK;
}
/** @name Platform-specific (user-implemented) functions */
//@{

/** @brief User-defined implementation for setting the given GPIO pin high. */
static void set_pin(const GPIO_Pin_t pin) {
    HAL_GPIO_WritePin(pin.port, pin.pin, GPIO_PIN_SET);
}

/** @brief User-defined implementation for setting the given GPIO pin low. */
static void reset_pin(const GPIO_Pin_t pin) {
    HAL_GPIO_WritePin(pin.port, pin.pin, GPIO_PIN_RESET);
}

/** @brief User-defined implementation for reading the state of the given GPIO pin. */
static bool read_pin(const GPIO_Pin_t pin) {
    return HAL_GPIO_ReadPin(pin.port, pin.pin);
}

/** @brief User-defined implementation for waiting until the controller's busy pin is released. */
static UC8253_Status_t wait_for_busy(const UC8253_Handle_t *hdrv) {
    while (read_pin(hdrv->busy) == 0);

    return UC8253_OK;
}

/** @brief User-defined implementation for a blocking delay, in milliseconds. */
static void delay_ms(const uint16_t delay) {
    HAL_Delay(delay);
}

/** @brief User-defined implementation for sending a single byte over SPI. */
static UC8253_Status_t spi_write(const UC8253_Handle_t *hdrv, const uint8_t data) {
    const HAL_StatusTypeDef status = HAL_SPI_Transmit(hdrv->hspi, &data, 1, 100);

    return status == HAL_OK ? UC8253_OK : UC8253_SPI_ERROR;
}
//@}