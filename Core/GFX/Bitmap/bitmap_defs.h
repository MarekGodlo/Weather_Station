/**
 * @file bitmap_defs.h
 *
 * Created on 8/21/26.
 * @author Marek Godlowicz https://github.com/MarekGodlo
 * @brief Bitmap data structure definitions for graphic rendering.
 */

#ifndef WEATHER_STATION_BITMAP_DEFS_H
#define WEATHER_STATION_BITMAP_DEFS_H
#include <stdint.h>

/**
 * @brief Defines a monochrome bitmap for EPD rendering.
 */
typedef struct {
    const uint8_t *data; /**< Pointer to the pixel data buffer. **/
    uint16_t width;      /**< Width of the bitmap, in pixels. **/
    uint16_t height;     /**< Height of the bitmap, in pixels. **/
} GFX_Bitmap_t;
#endif //WEATHER_STATION_BITMAP_DEFS_H
