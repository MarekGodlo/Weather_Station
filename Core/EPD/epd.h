/**
 * @file epd.h
 *
 * Created on 11.07.2026.
 * @author Marek Godlowicz https://github.com/MarekGodlo
 * @brief Display driver layer integrating UC8253 hardware control with
 *        GFX-based rendering and managing partial refresh regions.
 */

#ifndef WEATHER_STATION_EPD_H
#define WEATHER_STATION_EPD_H
#include <stdbool.h>

#include "EPD_Driver/uc8253.h"
#include "../GFX/gfx.h"

#define EPD_WIDTH 240                /**< EPD display width in pixels. */
#define EPD_HEIGHT 416               /**< EPD display height in pixels. */
#define EPD_SIZE ((EPD_WIDTH * EPD_HEIGHT)/8) /**< Total size of the frame buffer in bytes. */

#define EPD_PARTIAL_BUFFER_SIZE 12480 /**< Buffer size for partial display updates. */

/**
 * @brief Represents GPIO pins used by the EPD display.
 */
typedef struct {
    GPIO_Pin_t cs;   /**< Chip Select pin. */
    GPIO_Pin_t dc;   /**< Data/Command control pin. */
    GPIO_Pin_t busy; /**< Busy status pin. */
    GPIO_Pin_t res;  /**< Reset pin. */
} EPD_Pins_t;

/**
 * @brief Represents the status code returned by EPD driver functions.
 */
typedef enum {
    EPD_OK,              /**< Operation completed successfully. */
    EPD_NULL_ARG,        /**< A null pointer was passed as an argument. */
    EPD_BUFFER_OVERFLOW, /**< A buffer overflow occurred. */
    EPD_COMM_ERR         /**< Communication with the device failed. */
} EPD_Status_t;

/**
 * @brief Represents user-configurable settings, required by the driver.
 *
 * @note Buffers must remain valid for the lifetime of the EPD handle.
 */
typedef struct {
    uint8_t *frame_buffer;           /**< Pointer to the buffer holding the old (last displayed) image. */
    uint8_t *gfx_buffer;             /**< Pointer to the buffer holding the new (to be displayed) image. */
    uint16_t frame_size;             /**< Size of the frame buffers, in bytes. */
    UC8253_Handle_t hdrv;            /**< Low-level EPD driver handle. */
    GFX_Rotation_t display_rotation; /**< Desired display rotation. */
    bool skip_frame_buf_clr;         /**< If true, frame buffer clearing is skipped (used for retention). */
} EPD_Config_t;

/**
 * @brief Represents the EPD handle.
 */
typedef struct {
    uint8_t *frame_buffer;   /**< Pointer to the buffer holding the old (last displayed) image. */
    uint8_t *gfx_buffer;     /**< Pointer to the buffer holding the new (to be displayed) image. */
    uint16_t frame_size;     /**< Size of the frame buffers, in bytes. */

    UC8253_Handle_t hdrv;   /**< Low-level EPD driver handle. */
    GFX_Handle_t hgfx;      /**< GFX handle structure. */

    GFX_Rect_t dirty_region; /**< Region of the display that needs to be updated. */
    bool has_dirty_region;   /**< True if a dirty region is pending an update. */
} EPD_Handle_t;

/**
 * @brief Initializes the EPD handle.
 *
 * @param hepd   Pointer to the EPD handle.
 * @param config Pointer to the user-configurable settings.
 *
 * @return Status of the operation.
 */
EPD_Status_t EPD_Init(EPD_Handle_t *hepd, const EPD_Config_t *config);

/**
 * @brief Draws a single character on the display.
 *
 * @param hepd  Pointer to the EPD handle.
 * @param x     X-coordinate of the character's top-left corner.
 * @param y     Y-coordinate of the character's top-left corner.
 * @param c     Character to be drawn.
 * @param font  Pointer to the font structure.
 * @param color Color of the character.
 *
 * @return Status of the operation.
 */
EPD_Status_t EPD_DrawChar(EPD_Handle_t *hepd, int16_t x, int16_t y,
                          char c, const GFX_Font_t *font, uint8_t color);

/**
 * @brief Draws a single character with a background color on the display.
 *
 * @param hepd  Pointer to the EPD handle.
 * @param x     X-coordinate of the character's top-left corner.
 * @param y     Y-coordinate of the character's top-left corner.
 * @param c     Character to be drawn.
 * @param font  Pointer to the font structure.
 * @param color Color of the character.
 * @param bg    Background color.
 *
 * @return Status of the operation.
 */
EPD_Status_t EPD_DrawCharBg(EPD_Handle_t *hepd, int16_t x, int16_t y, char c,
                            const GFX_Font_t *font, uint8_t color, uint8_t bg);

/**
 * @brief Draws a text string on the display.
 *
 * @param hepd  Pointer to the EPD handle.
 * @param x     X-coordinate of the text's top-left corner.
 * @param y     Y-coordinate of the text's top-left corner.
 * @param text  String to be drawn.
 * @param font  Pointer to the font structure.
 * @param color Color of the text.
 *
 * @return Status of the operation.
 */
EPD_Status_t EPD_DrawText(EPD_Handle_t *hepd, int16_t x, int16_t y,
                          const char *text, const GFX_Font_t *font, uint8_t color);

/**
 * @brief Draws a text string with a background color on the display.
 *
 * @param hepd  Pointer to the EPD handle.
 * @param x     X-coordinate of the text's top-left corner.
 * @param y     Y-coordinate of the text's top-left corner.
 * @param text  String to be drawn.
 * @param font  Pointer to the font structure.
 * @param color Color of the text.
 * @param bg    Background color.
 *
 * @return Status of the operation.
 */
EPD_Status_t EPD_DrawTextBg(EPD_Handle_t *hepd, int16_t x, int16_t y,
                            const char *text, const GFX_Font_t *font, uint8_t color,
                            uint8_t bg);

/**
 * @brief Draws a vertical line on the display.
 *
 * @param hepd      Pointer to the EPD handle.
 * @param x         X-coordinate of the line's start.
 * @param y         Y-coordinate of the line's start.
 * @param len       Length of the line.
 * @param thickness Thickness of the line.
 * @param color     Color of the line.
 *
 * @return Status of the operation.
 */
EPD_Status_t EPD_DrawVLine(EPD_Handle_t *hepd, int16_t x, int16_t y,
                           uint16_t len, uint16_t thickness, uint8_t color);

/**
 * @brief Draws a horizontal line on the display.
 *
 * @param hepd      Pointer to the EPD handle.
 * @param x         X-coordinate of the line's start.
 * @param y         Y-coordinate of the line's start.
 * @param len       Length of the line.
 * @param thickness Thickness of the line.
 * @param color     Color of the line.
 *
 * @return Status of the operation.
 */
EPD_Status_t EPD_DrawHLine(EPD_Handle_t *hepd, int16_t x, int16_t y,
                           uint16_t len, uint16_t thickness, uint8_t color);

/**
 * @brief Draws a bitmap on the display.
 *
 * @param hepd   Pointer to the EPD handle.
 * @param x      X-coordinate of the bitmap's top-left corner.
 * @param y      Y-coordinate of the bitmap's top-left corner.
 * @param bitmap Pointer to the bitmap data.
 * @param w      Width of the bitmap.
 * @param h      Height of the bitmap.
 * @param color  Color of the bitmap.
 *
 * @return Status of the operation.
 */
EPD_Status_t EPD_DrawBitmap(EPD_Handle_t *hepd, int16_t x, int16_t y,
                            const uint8_t *bitmap, uint16_t w, uint16_t h,
                            uint8_t color);

/**
 * @brief Draws a bitmap with a background color on the display.
 *
 * @param hepd   Pointer to the EPD handle.
 * @param x      X-coordinate of the bitmap's top-left corner.
 * @param y      Y-coordinate of the bitmap's top-left corner.
 * @param bitmap Pointer to the bitmap data.
 * @param w      Width of the bitmap.
 * @param h      Height of the bitmap.
 * @param color  Color of the bitmap.
 * @param bg     Background color.
 *
 * @return Status of the operation.
 */
EPD_Status_t EPD_DrawBitmapBg(EPD_Handle_t *hepd, int16_t x, int16_t y,
                              const uint8_t *bitmap, uint16_t w, uint16_t h,
                              uint8_t color, uint8_t bg);


/**
 * @brief Fills a rectangular region on the display with a solid color.
 *
 * @param hepd  Pointer to the EPD handle.
 * @param x     X-coordinate of the region's top-left corner.
 * @param y     Y-coordinate of the region's top-left corner.
 * @param w     Width of the region, in pixels.
 * @param h     Height of the region, in pixels.
 * @param color Fill color.
 *
 * @return Status of the operation.
 */
EPD_Status_t EPD_FillRegion(EPD_Handle_t *hepd, int16_t x, int16_t y,
                            uint16_t w, uint16_t h, uint8_t color);

/**
 * @brief Updates the entire display with the contents of the GFX buffer.
 *
 * @param hepd Pointer to the EPD handle.
 *
 * @return Status of the operation.
 */
EPD_Status_t EPD_UpdateDisplay(EPD_Handle_t *hepd);

/**
 * @brief Performs a partial update of the display in the dirty region.
 *
 * @param hepd Pointer to the EPD handle.
 *
 * @return Status of the operation.
 */
EPD_Status_t EPD_UpdateDisplayPartial(EPD_Handle_t *hepd);
#endif //WEATHER_STATION_EPD_H
