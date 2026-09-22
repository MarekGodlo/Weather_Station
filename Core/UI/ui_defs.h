/**
 * @file ui_defs.h
 *
 * Created on 9/6/26.
 * @author Marek Godlowicz https://github.com/MarekGodlo
 * @brief Static layout data for the weather station UI (positions, fonts, backgrounds).
 */

#ifndef WEATHER_STATION_UI_DEFS_H
#define WEATHER_STATION_UI_DEFS_H
#include "stdint.h"

#include "../GFX/Font/FreeSans18pt8b.h"

/**
 * @brief Placeholder string displayed when a value pointer is NULL.
 */
#define UI_VALUE_ERR_MSG "N/A"

/**
 * @brief Represents a 2D position on the screen.
 */
typedef struct {
    int16_t x; /**< X coordinate. */
    int16_t y; /**< Y coordinate. */
} UI_Pos_t;

/**
 * @brief Layout configuration for static text labels.
 */
typedef struct {
    UI_Pos_t pos;           /**< Position of the label. */
    const GFX_Font_t *font; /**< Font used for the label. */
    const char *text;       /**< Text string to display. */
} UI_StaticLabel_t;

/**
 * @brief Orientation of a UI line.
 */
typedef enum {
    UI_LINE_HORIZONTAL,
    UI_LINE_VERTICAL,
} UI_LineOrient_t;

/**
 * @brief Configuration for drawing a line on the UI.
 */
typedef struct {
    UI_Pos_t pos;           /**< Starting position. */
    uint16_t len;           /**< Length of the line. */
    uint16_t thickness;     /**< Thickness in pixels. */
    UI_LineOrient_t orient; /**< Horizontal or vertical orientation. */
    uint8_t color;          /**< Color of the line. */
} UI_Line_t;

/**
 * @brief Global UI background and static lines configuration.
 */
typedef struct {
    const UI_Line_t *lines; /**< Array of background lines. */
    uint8_t line_count;     /**< Number of lines in the array. */
    uint8_t fg_color;       /**< Default foreground color. */
    uint8_t bg_color;       /**< Default background color. */
} UI_Background_t;

/**
 * @brief IDs for static text labels.
 * @note Used as indexes for ui_static_labels array.
 */
typedef enum {
    UI_LABEL_IN = 0,
    UI_LABEL_OUT,
    UI_LABEL_COUNT
} UI_StaticLabelID_t;

/**
 * @brief IDs for static icons.
 * @note Used as indexes for ui_static_icons array.
 */
typedef enum {
    UI_ICON_TEMP_IN = 0,
    UI_ICON_DROPLET,
    UI_ICON_GAUGE,
    UI_ICON_TEMP_OUT,
    UI_ICON_COUNT
} UI_StaticIcon_t;

/**
 * @brief IDs for dynamic UI elements (sensor values).
 * @note Used as indexes for ui_layout_dynamic array.
 */
typedef enum {
    UI_DYN_TEMP_IN = 0,
    UI_DYN_HUM_IN,
    UI_DYN_PRESS_IN,
    UI_DYN_TEMP_OUT,
    UI_DYN_ITEM_COUNT
} UI_DynamicItem_t;

/**
 * @brief Layout and clearing box for dynamic text values.
 */
typedef struct {
    const GFX_Font_t *font; /**< Font used for the value. */
    uint16_t width;         /**< Width of the clearing box. */
    uint16_t height;        /**< Height of the clearing box. */
    int16_t y_offset;       /**< Y offset for the clearing box relative to text position. */
} UI_TextLayout_t;

/**
 * @brief Definitions of static background lines.
 */
static const UI_Line_t ui_bg_lines[] = {
    {.pos = {208, 0}, 240, 2, UI_LINE_VERTICAL, GFX_COLOR_BLACK},
    {.pos = {0, 50}, 416, 2, UI_LINE_HORIZONTAL, GFX_COLOR_BLACK}
};

/**
 * @brief Global UI background configuration.
 */
static const UI_Background_t ui_background = {
    .lines = ui_bg_lines,
    .line_count = sizeof(ui_bg_lines) / sizeof(UI_Line_t),
    .fg_color = GFX_COLOR_BLACK,
    .bg_color = GFX_COLOR_WHITE
};

/**
 * @brief Positions and content for static labels (e.g. "In", "Out").
 */
static const UI_StaticLabel_t ui_static_labels[UI_LABEL_COUNT] = {
    [UI_LABEL_IN] = {{10, 40}, &FreeSans18pt8b, "In"},
    [UI_LABEL_OUT] = {{220, 40}, &FreeSans18pt8b, "Out"}
};

/**
 * @brief Positions for static icons on the screen.
 */
static const UI_Pos_t ui_static_icons[UI_ICON_COUNT] = {
    [UI_ICON_TEMP_IN] = {0, 60},
    [UI_ICON_DROPLET] = {0, 110},
    [UI_ICON_GAUGE] = {0, 160},
    [UI_ICON_TEMP_OUT] = {210, 60}
};

/**
 * @brief Screen positions for dynamic sensor values.
 */
static const UI_Pos_t ui_layout_dynamic[UI_DYN_ITEM_COUNT] = {
    [UI_DYN_TEMP_IN] = {40, 90},
    [UI_DYN_HUM_IN] = {40, 140},
    [UI_DYN_PRESS_IN] = {40, 190},
    [UI_DYN_TEMP_OUT] = {250, 90}
};

/**
 * @brief Font and box configuration for rendering sensor values.
 */
static const UI_TextLayout_t value_layout = {
    .font = &FreeSans18pt8b,
    .width = 160,
    .height = 35,
    .y_offset = -30
};
#endif //WEATHER_STATION_UI_DEFS_H
