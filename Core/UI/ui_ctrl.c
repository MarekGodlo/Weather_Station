/**
 * @file ui_ctrl.c
 *
 * Created on 8/21/26.
 * @author Marek Godlowicz https://github.com/MarekGodlo
 * @brief Wraps the EPD module to manage the UI shown on the e-paper display.
 */

#include "ui_ctrl.h"
#include "ui_defs.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "../GFX/Bitmap/icons.h"

#define FLOAT_TO_DECI(_f_val) ((int16_t)((_f_val) * 10.0f))
#define FLOAT_TO_CENTI(_f_val) ((int16_t)((_f_val) * 100.0f))
#define PA_TO_HPA_INT(_f_val) ((int32_t)((_f_val) / 100.0f))

static bool validate_config(const EPD_Config_t *config);

static void clear_text_box(const UI_Ctrl_Handle_t *hui, int16_t text_x, int16_t text_y, UI_TextLayout_t text_box);

static void display_temp_value(UI_Ctrl_Handle_t *hui, UI_Pos_t value_pos, int16_t centi_degree);
static void display_hum_value(UI_Ctrl_Handle_t *hui, UI_Pos_t value_pos, int16_t deci_degree);
static void display_pres_value(UI_Ctrl_Handle_t *hui, UI_Pos_t value_pos, uint16_t hpa);
static void display_error_value(UI_Ctrl_Handle_t *hui, UI_Pos_t value_pos, const char *error_msg);

static void display_temp_icon(const UI_Ctrl_Handle_t *hui, UI_Pos_t icon_pos);
static void display_droplet_icon(const UI_Ctrl_Handle_t *hui, UI_Pos_t icon_pos);
static void display_gauge_icon(const UI_Ctrl_Handle_t *hui, UI_Pos_t icon_pos);

static void display_background_lines(UI_Ctrl_Handle_t *hui);

static void display_text(const UI_Ctrl_Handle_t *hui, UI_StaticLabel_t label);

static void display_line(const UI_Ctrl_Handle_t *hui, UI_Line_t line);
static void display_value(UI_Ctrl_Handle_t *hui, int16_t x, int16_t y, UI_TextLayout_t text_box, const char *format, ...);

UI_Ctrl_Status_t UI_Ctrl_Init(const UI_Ctrl_Handle_t *hui, const EPD_Config_t *config) {
     assert_param(hui != NULL);
     assert_param(config != NULL);

    if (hui == NULL || !validate_config(config)) {
        return UI_CTRL_NULL_ARG;
    }

     if (EPD_Init(hui->hepd, config) != EPD_OK) {
         return UI_CTRL_INIT_ERR;
     };

    return UI_CTRL_OK;
}

void UI_Ctrl_DisplayStaticElements(UI_Ctrl_Handle_t *hui) {
    assert_param(hui != NULL);

    if (hui == NULL) {
        return;
    }

    display_background_lines(hui);

    display_text(hui, ui_static_labels[UI_LABEL_IN]);
    display_text(hui, ui_static_labels[UI_LABEL_OUT]);

    display_temp_icon(hui, ui_static_icons[UI_ICON_TEMP_IN]);
    display_droplet_icon(hui, ui_static_icons[UI_ICON_DROPLET]);
    display_gauge_icon(hui, ui_static_icons[UI_ICON_GAUGE]);

    display_temp_icon(hui, ui_static_icons[UI_ICON_TEMP_OUT]);
}

void UI_Ctrl_DisplayValues(UI_Ctrl_Handle_t *hui, const float *temp_in, const float *humidity_in, const uint32_t *pressure_in, const float *temp_out) {
    assert_param(hui != NULL);

    if (hui == NULL) {
        return;
    }

    if (temp_in) display_temp_value(hui, ui_layout_dynamic[UI_DYN_TEMP_IN], FLOAT_TO_CENTI(*temp_in));
    else display_error_value(hui, ui_layout_dynamic[UI_DYN_TEMP_IN], UI_VALUE_ERR_MSG);

    if (humidity_in) display_hum_value(hui, ui_layout_dynamic[UI_DYN_HUM_IN], FLOAT_TO_DECI(*humidity_in));
    else display_error_value(hui, ui_layout_dynamic[UI_DYN_HUM_IN], UI_VALUE_ERR_MSG);

    if (pressure_in) display_pres_value(hui, ui_layout_dynamic[UI_DYN_PRESS_IN], PA_TO_HPA_INT(*pressure_in));
    else display_error_value(hui, ui_layout_dynamic[UI_DYN_PRESS_IN], UI_VALUE_ERR_MSG);

    if (temp_out) display_temp_value(hui, ui_layout_dynamic[UI_DYN_TEMP_OUT], FLOAT_TO_CENTI(*temp_out));
    else display_error_value(hui, ui_layout_dynamic[UI_DYN_TEMP_OUT], UI_VALUE_ERR_MSG);
}

UI_Ctrl_Status_t UI_Ctrl_Update(const UI_Ctrl_Handle_t *hui) {
    assert_param(hui != NULL);

    if (hui == NULL) {
        return UI_CTRL_NULL_ARG;
    }

    if (EPD_UpdateDisplay(hui->hepd) != EPD_OK) {
        return UI_CTRL_UPDATE_ERR;
    }

    return UI_CTRL_OK;
}

UI_Ctrl_Status_t UI_Ctrl_UpdatePartial(const UI_Ctrl_Handle_t *hui) {
    assert_param(hui != NULL);

    if (hui == NULL) {
        return UI_CTRL_NULL_ARG;
    }

    if (EPD_UpdateDisplayPartial(hui->hepd) != EPD_OK) {
        return UI_CTRL_UPDATE_ERR;
    };

    return UI_CTRL_OK;
}

static bool validate_config(const EPD_Config_t *config) {
    if (config == NULL) return false;
    if (config->gfx_buffer == NULL) return false;
    if (config->frame_buffer == NULL) return false;
    return true;
}

static void display_background_lines(UI_Ctrl_Handle_t *hui) {
    for (int i = 0; i < ui_background.line_count; ++i) {
        display_line(hui, ui_background.lines[i]);
    }
}

static void display_line(const UI_Ctrl_Handle_t *hui, const UI_Line_t line) {
    if (line.orient == UI_LINE_HORIZONTAL) {
        EPD_DrawHLine(hui->hepd, line.pos.x, line.pos.y, line.len, line.thickness, line.color);
    } else {
        EPD_DrawVLine(hui->hepd, line.pos.x, line.pos.y, line.len, line.thickness, line.color);
    }
}

static void display_temp_value(UI_Ctrl_Handle_t *hui, const UI_Pos_t value_pos, const int16_t centi_degree) {
    const int16_t int_part = centi_degree / 100;
    const uint16_t frac_part = abs(centi_degree % 100);

    display_value(hui, value_pos.x, value_pos.y, value_layout, "%0d.%02d\xB0\x43", int_part, frac_part);
}

static void display_hum_value(UI_Ctrl_Handle_t *hui, const UI_Pos_t value_pos, const int16_t deci_degree) {
    const int16_t int_part = deci_degree / 10;
    const uint16_t frac_part = abs(deci_degree % 10);

    display_value(hui, value_pos.x, value_pos.y, value_layout, "%0d.%0d%%", int_part, frac_part);
}

static void display_pres_value(UI_Ctrl_Handle_t *hui, const UI_Pos_t value_pos, const uint16_t hpa) {
    display_value(hui, value_pos.x, value_pos.y, value_layout, "%dhPa", hpa);
}

static void display_error_value(UI_Ctrl_Handle_t *hui, const UI_Pos_t value_pos, const char *error_msg) {
    display_value(hui, value_pos.x, value_pos.y, value_layout, error_msg);
}

static void display_temp_icon(const UI_Ctrl_Handle_t *hui, const UI_Pos_t icon_pos) {
    EPD_DrawBitmap(hui->hepd, icon_pos.x, icon_pos.y, temp_bitmap.data, temp_bitmap.width, temp_bitmap.height, ui_background.fg_color);
}

static void display_droplet_icon(const UI_Ctrl_Handle_t *hui, const UI_Pos_t icon_pos) {
    EPD_DrawBitmap(hui->hepd, icon_pos.x, icon_pos.y, droplet_bitmap.data, droplet_bitmap.width, droplet_bitmap.height, ui_background.fg_color);
}

static void display_gauge_icon(const UI_Ctrl_Handle_t *hui, const UI_Pos_t icon_pos) {
    EPD_DrawBitmap(hui->hepd, icon_pos.x, icon_pos.y, gauge_bitmap.data, gauge_bitmap.width, gauge_bitmap.height, ui_background.fg_color);
}

static void display_text(const UI_Ctrl_Handle_t *hui, const UI_StaticLabel_t label) {
    EPD_DrawText(hui->hepd, label.pos.x, label.pos.y, label.text, label.font, ui_background.fg_color);
}

static void clear_text_box(const UI_Ctrl_Handle_t *hui, const int16_t text_x, const int16_t text_y, const UI_TextLayout_t text_box) {
    EPD_FillRegion(hui->hepd, text_x, text_y + text_box.y_offset, text_box.width, text_box.height, ui_background.bg_color);
}

static void display_value(UI_Ctrl_Handle_t *hui, const int16_t x, const int16_t y, const UI_TextLayout_t text_box, const char *format, ...) {
    char buff[32];
    va_list args;

    va_start(args, format);
    vsnprintf(buff, sizeof(buff), format, args);
    va_end(args);

    clear_text_box(hui, x, y, text_box);
    EPD_DrawText(hui->hepd, x, y, buff, text_box.font, ui_background.fg_color);
}