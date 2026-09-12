/**
 * @file font_defs.h
 *
 * Created on 8/12/26.
 * @author Marek Godlowicz https://github.com/MarekGodlo
 * @brief Font and glyph data structure definitions for graphic rendering.
 */

#ifndef WEATHER_STATION_FONT_DEFS_H
#define WEATHER_STATION_FONT_DEFS_H
#include <stdint.h>

/**
 * @brief Defines a single glyph within a font.
 */
typedef struct {
    uint16_t bitmap_offset; /**< Offset into the font's bitmap array where this glyph's data begins. */
    uint8_t width;          /**< Glyph bitmap width, in pixels. */
    uint8_t height;         /**< Glyph bitmap height, in pixels. */
    uint8_t x_advance;      /**< Distance to advance the cursor horizontally after drawing this glyph. */
    int8_t x_offset;        /**< Horizontal offset from the cursor to the glyph's top-left corner. */
    int8_t y_offset;        /**< Vertical offset from the cursor to the glyph's top-left corner. */
} GFX_Glyph_t;

/**
 * @brief Defines a font as a set of glyphs and their bitmap data.
 */
typedef struct {
    uint8_t *bitmap;     /**< Pointer to the concatenated bitmap data for all glyphs. */
    GFX_Glyph_t *glyph;  /**< Pointer to the array of glyph descriptors. */
    uint16_t first;      /**< Codepoint of the first character in the font. */
    uint16_t last;       /**< Codepoint of the last character in the font. */
    uint8_t y_advance;   /**< Vertical distance to advance the cursor for a new line. */
} GFX_Font_t;
#endif //WEATHER_STATION_FONT_DEFS_H
