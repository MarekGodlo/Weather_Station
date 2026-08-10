/**
 * @file uc8253.h
 *
 * Created on 19.06.2026.
 * @author Marek Godlowicz https://github.com/MarekGodlo
 * @brief Low-level driver for the UC8253 e-paper display controller.
 */

#ifndef EPD_UC8253_H
#define EPD_UC8253_H
#include "main.h"

#define UC8253_PANEL_SETTING 0x00
#define UC8253_CDI 0x50

#define UC8253_PARTIAL_WINDOW 0x90
#define UC8253_PARTIAL_IN 0x91
#define UC8253_PARTIAL_OUT 0x92

#define UC8253_DEEP_SLEEP 0x07

#define UC8253_START_TX1 0x10
#define UC8253_START_TX2 0x13

#define UC8253_POWER_ON 0x04
#define UC8253_POWER_OFF 0x02

#define UC8253_DISPLAY_REFRESH 0x12 // 200us delay

#define UC8253_CASCADE_SETTING 0xE0

#define UC8253_FORCE_TEMP 0xE5

/**
 * @brief Calculates the required image buffer size, in bytes.
 *
 * @param _width  Display width, in pixels.
 * @param _height Display height, in pixels.
 */
#define UC8253_CALC_BUF_SIZE(_width, _height) (((_width)/8) * (_height))

/**
 * @brief Represents the status of the controller's operations.
 */
typedef enum {
    UC8253_OK,        /**< Successful. */
    UC8253_TIMEOUT,   /**< Operation timed out while waiting for a busy signal. */
    UC8253_BUSY,      /**< Controller is busy and cannot accept a new operation. */
    UC8253_SPI_ERROR, /**< SPI communication failed. */
    UC8253_NULL_ARG   /**< A required argument was NULL. */
} UC8253_Status_t;

/**
 * @brief Represents the UC8253 controller handle.
 */
typedef struct {
    GPIO_Pin_t res;  /**< Reset pin. */
    GPIO_Pin_t cs;   /**< Chip select pin. */
    GPIO_Pin_t dc;   /**< Data/command pin. */
    GPIO_Pin_t busy; /**< Busy status pin. */
    uint16_t width;  /**< Display width, in pixels. */
    uint16_t height; /**< Display height, in pixels. */
    void *hspi;      /**< User implementation of the SPI handler. */
} UC8253_Handle_t;

/** @name Initialization functions */
//@{
/**
 * @brief Initializes the UC8253 controller and configures it for full refresh mode.
 *
 * @warning Enables the display's power-on sequence.
 *
 * @param hdrv Pointer to the UC8253 handle structure.
 * @return     UC8253_OK on success, error status otherwise.
 */
UC8253_Status_t UC8253_Init(UC8253_Handle_t *hdrv);

/**
 * @brief Initializes the UC8253 controller and configures it for partial refresh mode.
 *
 * @warning Enables the display's power-on sequence.
 *
 * @param hdrv Pointer to the UC8253 handle structure.
 * @return UC8253_OK on success, error status otherwise.
 */
UC8253_Status_t UC8253_Init_Partial(UC8253_Handle_t *hdrv);
//}@

/** @name Writing data functions */
//@{
/**
 * @brief Sends new image data into the controller's SRAM memory.
 *
 * @param hdrv    Pointer to the UC8253 handle structure.
 * @param size    Size of the image data, in bytes.
 * @param new_img Pointer to the image data buffer.
 * @return UC8253_OK on success, error status otherwise.
 */
UC8253_Status_t UC8253_WriteImgNew(UC8253_Handle_t *hdrv, uint16_t size, const uint8_t *new_img);

/**
 * @brief Sends old image data into the controller's SRAM memory.
 * @note Required during partial refresh mode.
 *
 * @param hdrv    Pointer to the UC8253 handle structure.
 * @param size    Size of the image data, in bytes.
 * @param old_img Pointer to the image data buffer.
 * @return UC8253_OK on success, error status otherwise.
 */
UC8253_Status_t UC8253_WriteImgOld(UC8253_Handle_t *hdrv, uint16_t size, const uint8_t *old_img);
//}@

/** @name Configuration functions */
//@{
/**
 * @brief Enters partial refresh mode.
 *
 * @param hdrv    Pointer to the UC8253 handle structure.
 * @param x_start Starting horizontal position of the region to update.
 * @param y_start Starting vertical position of the region to update.
 * @param width   Width of the region to update.
 * @param height  Height of the region to update.
 * @return UC8253_OK on success, error status otherwise.
 */
UC8253_Status_t UC8253_EnterPartialMode(UC8253_Handle_t *hdrv, uint16_t x_start, uint16_t y_start,
                                        uint16_t width, uint16_t height);

/**
 * @brief Refreshes the display.
 *
 * @param hdrv Pointer to the UC8253 handle structure.
 * @return UC8253_OK on success, error status otherwise.
 */
UC8253_Status_t UC8253_UpdateDisplay(UC8253_Handle_t *hdrv);

/**
 * @brief Enters deep sleep mode.
 *
 * @note Also sends the power-off command.
 *
 * @param hdrv Pointer to the UC8253 handle structure.
 * @return UC8253_OK on success, error status otherwise.
 */
UC8253_Status_t UC8253_EnterDeepSleep(UC8253_Handle_t *hdrv);

/**
 * @brief Disables the display's power-on sequence
 *
 * @param hdrv Pointer to the UC8253 handle structure.
 * @return UC8253_OK on success, error status otherwise.
 */
UC8253_Status_t UC8253_PowerOff(UC8253_Handle_t *hdrv);

/**
 * @brief Performs a hard reset of the display.
 *
 * @param hdrv Pointer to the UC8253 handle structure.
 */
void UC8253_Reset(UC8253_Handle_t *hdrv);

/**
 * @brief Exits partial refresh mode.
 *
 * @param hdrv Pointer to the UC8253 handle structure.
 * @return UC8253_OK on success, error status otherwise.
 */
UC8253_Status_t UC8253_PartialOut(UC8253_Handle_t *hdrv);
//}@

#endif //EPD_UC8253_H
