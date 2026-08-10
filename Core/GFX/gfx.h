/**
 * @file gfx.h
 *
 * Created on 11.07.2026.
 * @author Marek Godlowicz https://github.com/MarekGodlo
 * @brief Core graphics library providing common drawing primitives
 *        (points, lines, bitmap, etc.), ported to plain C.
 *        Based on the Adafruit GFX Library.
 */

/*
This is the core graphics library for all our displays, providing a common
set of graphics primitives (points, lines, circles, etc.).  It needs to be
paired with a hardware-specific library for each display device we carry
(to handle the lower-level functions).

Adafruit invests time and resources providing this open source code, please
support Adafruit & open-source hardware by purchasing products from Adafruit!

Copyright (c) 2013 Adafruit Industries.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

- Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef GFX_ARDUINO_LIKE_GFX_H
#define GFX_ARDUINO_LIKE_GFX_H
#include "main.h"
#include "../GFX/Font/font_defs.h"

#define GFX_COLOR_WHITE 0xFF
#define GFX_COLOR_BLACK 0x00

/**
 * @brief Represents the display rotation angle.
 */
typedef enum {
    GFX_ROTATION_0 = 0, /**< No rotation (0 degrees). */
    GFX_ROTATION_90,    /**< 90 degrees rotation. */
    GFX_ROTATION_180,   /**< 180 degrees rotation. */
    GFX_ROTATION_270    /**< 270 degrees rotation. */
} GFX_Rotation_t;

/**
 * @brief Represents a rectangular area defined by position and size.
 */
typedef struct {
    uint16_t x;  /**< Horizontal position, in pixels. */
    uint16_t y;  /**< Vertical position, in pixels. */
    uint16_t w;  /**< Width of the rectangle, in pixels. */
    uint16_t h;  /**< Height of the rectangle, in pixels. */
} GFX_Rect_t;

/**
 * @brief Represents user-configurable display settings.
 */
typedef struct {
    uint16_t width;  /**< Physical width of the display, in pixels. */
    uint16_t height; /**< Physical height of the display, in pixels. */
} GFX_Config_t;

/**
 * @brief Represents the GFX handle.
 */
typedef struct {
    GFX_Config_t config;     /**< User-configurable display settings. */

    uint16_t vr_width;       /**< Virtual width of the frame, in pixels. */
    uint16_t vr_height;      /**< Virtual height of the frame, in pixels. */
    GFX_Rotation_t rotation; /**< Current display rotation. */
} GFX_Handle_t;

/**
 * @brief Initializes the GFX handle.
 * The contents of @p config are copied into @p hgfx.
 *
 * @param hgfx   Pointer to the GFX handle structure.
 * @param config Pointer to the user-configurable display settings.
 */
void GFX_Init(GFX_Handle_t *hgfx, const GFX_Config_t *config);

/**
 * @brief Sets the display rotation.
 *
 * @param hgfx     Pointer to the GFX handle structure.
 * @param rotation Desired display rotation.
 */
void GFX_SetRotation(GFX_Handle_t *hgfx, GFX_Rotation_t rotation);

/**
 * @brief Draws a pixel to the canvas frame buffer
 *
 * @param hgfx       Pointer to the GFX handle structure.
 * @param gfx_buffer Pointer to the frame buffer.
 * @param x          x coordinate.
 * @param y          y coordinate.
 * @param color      Pixel color (black or white).
 */
void GFX_DrawPixel(const GFX_Handle_t *hgfx, uint8_t *gfx_buffer,
                   int16_t x, int16_t y, uint8_t color);

/**
 * @brief Draws a horizontal line at given coordinates in the frame buffer.
 *
 * @note Coordinates are the beginning of the line's starting point.
 *
 * @param hgfx       Pointer to the GFX handle structure.
 * @param gfx_buffer Pointer to the frame buffer.
 * @param x          Horizontal position of the line, in pixels.
 * @param y          Vertical position of the line, in pixels.
 * @param len        Length of the line in pixels.
 * @param thickness  Thickness of the line, in pixels.
 * @param color      Line color.
 */
void GFX_DrawHLine(const GFX_Handle_t *hgfx, uint8_t *gfx_buffer,
                   int16_t x, int16_t y, uint16_t len,
                   uint16_t thickness, uint8_t color);

/**
 * @brief Draws a vertical line at given coordinates in the frame buffer.
 *
 * @note Coordinates are the beginning of the line's starting point.
 *
 * @param hgfx       Pointer to the GFX handle structure.
 * @param gfx_buffer Pointer to the frame buffer.
 * @param x          Horizontal position of the line, in pixels.
 * @param y          Vertical position of the line, in pixels.
 * @param len        Length of the line, in pixels.
 * @param thickness  Thickness of the line, in pixels.
 * @param color      Line color.
 */
void GFX_DrawVLine(const GFX_Handle_t *hgfx, uint8_t *gfx_buffer,
                   int16_t x, int16_t y, uint16_t len,
                   uint16_t thickness, uint8_t color);

/**
 * @brief Draws a character at given coordinates in the frame buffer.
 *
 * @param hgfx       Pointer to the GFX handle structure.
 * @param gfx_buffer Pointer to the frame buffer.
 * @param x          Horizontal position of the char, in pixels.
 * @param y          Vertical position of the char, in pixels.
 * @param c          Character code to draw.
 * @param font       Pointer to the font used to render the character.
 * @param color      Color used to draw the character.
 */
void GFX_DrawChar(const GFX_Handle_t *hgfx, uint8_t *gfx_buffer,
                  int16_t x, int16_t y, uint8_t c, const GFX_Font_t *font,
                  uint8_t color);

/**
 * @brief Draws a text at given coordinates in the frame buffer.
 *
 * @param hgfx       Pointer to the GFX handle structure.
 * @param gfx_buffer Pointer to the frame buffer.
 * @param x          Horizontal position of the text, in pixels.
 * @param y          Vertical position of the char, in pixels.
 * @param text       Pointer to the text.
 * @param font       Pointer to the font used to render the text.
 * @param color      Color used to draw the text.
 */
void GFX_DrawText(const GFX_Handle_t *hgfx, uint8_t *gfx_buffer,
                  int16_t x, int16_t y, const char *text,
                  const GFX_Font_t *font, uint8_t color);

/**
 * @brief Calculates the bounding rectangle of a character at the given cursor position.
 *
 * @param font     Pointer to the font used to render the character.
 * @param c        Character code to measure.
 * @param cursor_x Horizontal cursor position, in pixels.
 * @param cursor_y Vertical cursor position, in pixels.
 * @param out_rect Pointer to the rectangle that receives the calculated bounds.
 */
void GFX_GetCharBounds(const GFX_Font_t *font, char c,
                       int16_t cursor_x, int16_t cursor_y,
                       GFX_Rect_t *out_rect);

/**
 * @brief Calculates the bounding rectangle of a text at the given cursor position.
 *
 * @param font     Pointer to the font used to render the text.
 * @param text     Pointer to the text.
 * @param cursor_x Horizontal cursor position, in pixels.
 * @param cursor_y Vertical cursor position, in pixels.
 * @param out_rect Pointer to the rectangle that receives the calculated bounds.
 */
void GFX_GetTextBounds(const GFX_Font_t *font, const char *text, int16_t cursor_x, int16_t cursor_y, GFX_Rect_t *out_rect);

/**
 *   @bref Draws a 1-bit image at the specified (x,y) position, using the specified foreground color (unset bits are transparent).
 *
 *   @param hgfx       Pointer to the GFX handle structure.
 *   @param gfx_buffer Pointer to the frame buffer.
 *   @param x          Top left corner x coordinate.
 *   @param y          Top left corner y coordinate.
 *   @param bitmap     byte array with monochrome bitmap.
 *   @param w          Width of bitmap, in pixels.
 *   @param h          Height of bitmap, in pixels.
 *   @param color      Pixel color to draw with (black or white).
 */
void GFX_DrawBitmap(const GFX_Handle_t *hgfx, uint8_t *gfx_buffer, int16_t x, int16_t y,
                    const uint8_t *bitmap, uint16_t w, uint16_t h,
                    uint8_t color);

/**
 * @brief Draws a 1-bit image at the specified (x,y) position, using the specified foreground (for set bits) and background (unset bits) colors.
 *
 * @param hgfx       Pointer to the GFX handle structure.
 * @param gfx_buffer Pointer to the frame buffer.
 * @param x          Top left corner x coordinate.
 * @param y          Top left corner y coordinate.
 * @param bitmap     byte array with monochrome bitmap.
 * @param w          Width of bitmap, in pixels.
 * @param h          Height of bitmap, in pixels.
 * @param color      Pixel color to draw with (black or white).
 * @param bg         Background color to fill unset bits with (black or white).
 */
void GFX_DrawBitmapBg(const GFX_Handle_t *hgfx, uint8_t *gfx_buffer, int16_t x, int16_t y,
                      const uint8_t *bitmap, uint16_t w, uint16_t h,
                      uint8_t color, uint8_t bg);

/**
 * @brief Fills a rectangular region with the specified color.
 *
 * @param hgfx       Pointer to the GFX handle structure.
 * @param gfx_buffer Pointer to the frame buffer.
 * @param rect       Rectangle to fill.
 * @param color      Pixel color to draw with.
 */
void GFX_FillRect(const GFX_Handle_t *hgfx, uint8_t *gfx_buffer, GFX_Rect_t rect, uint8_t color);

/**
 * @brief Extracts a rectangular region from the frame buffer into a destination buffer.
 *
 * @param src         Pointer to the source buffer.
 * @param full_width  Width of the source buffer, in pixels.
 * @param region      Rectangle defining the area to extract.
 * @param dst         Pointer to the destination buffer that receives the extracted region.
 */
void GFX_ExtractRegion(const uint8_t *src, uint16_t full_width,
                       GFX_Rect_t region, uint8_t *dst);

/**
 * @brief Commits a rectangular region into the destination buffer.
 *
 * @param dst_full    Pointer to the destination buffer.
 * @param full_width  Width of the destination buffer, in pixels.
 * @param region      Rectangle defining the area to write into.
 * @param src_region  Pointer to the source buffer containing the region to commit.
 */
void GFX_CommitRegion(uint8_t *dst_full, uint16_t full_width,
                       GFX_Rect_t region, const uint8_t *src_region);

/**
 * @brief Translates a region from display (rotated) coordinates to raw
 *        (unrotated) frame buffer coordinates, aligned to byte boundaries.
 *
 * @param hgfx       Pointer to the GFX handle structure.
 * @param region     Rectangle in display coordinates.
 * @param raw_region Pointer to the rectangle that receives the translated,
 *                   byte-aligned coordinates in the raw frame buffer.
 */
void GFX_GetRawRegion(const GFX_Handle_t *hgfx, GFX_Rect_t region, GFX_Rect_t *raw_region);

#endif //GFX_ARDUINO_LIKE_GFX_H