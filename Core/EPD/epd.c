/**
 * @file epd.c
 *
 * Created on 11.07.2026.
 * @author Marek Godlowicz https://github.com/MarekGodlo
 * @brief Display driver layer integrating UC8253 hardware control with
 *        GFX-based rendering and managing partial refresh regions.
 */

#include "epd.h"

#include <string.h>

static uint8_t region_buffer[EPD_PARTIAL_BUFFER_SIZE];

#define CHECK_STATUS_UC8253(_expr) do { \
    const UC8253_Status_t _s = (_expr); \
    if (_s != UC8253_OK) { \
        return EPD_COMM_ERR; \
    } \
} while (0) \

static EPD_Status_t update_region(EPD_Handle_t *hepd, GFX_Rect_t region);
static void expand_dirty_region(EPD_Handle_t *hepd, GFX_Rect_t region);

static EPD_Status_t refresh_display(EPD_Handle_t *hepd);
static EPD_Status_t refresh_display_partial(EPD_Handle_t *hepd, uint16_t region_size, GFX_Rect_t region);

EPD_Status_t EPD_Init(EPD_Handle_t *hepd, const EPD_Config_t *config) {
    if (hepd == NULL || config == NULL) {
        return EPD_NULL_ARG;
    }
    
    assert_param(config->frame_buffer != NULL);
    assert_param(config->gfx_buffer != NULL);
    assert_param(config->hdrv != NULL);
    
    const GFX_Config_t gfx_config = {
        .width = config->hdrv.width,
        .height = config->hdrv.height,
    };
    
    GFX_Init(&hepd->hgfx, &gfx_config);
    GFX_SetRotation(&hepd->hgfx, config->display_rotation);

    hepd->frame_buffer = config->frame_buffer;
    hepd->gfx_buffer = config->gfx_buffer;
    hepd->frame_size = config->frame_size;
    hepd->hdrv = config->hdrv;

    memset(hepd->frame_buffer, 0xFF, hepd->frame_size);
    memset(hepd->gfx_buffer, 0xFF, hepd->frame_size);

    hepd->dirty_region = (GFX_Rect_t) {0,0,0,0};
    hepd->has_dirty_region = false;

    return EPD_OK;
}

EPD_Status_t EPD_DrawHLine(EPD_Handle_t *hepd, const int16_t x, const int16_t y,
                           const uint16_t len, const uint16_t thickness, const uint8_t color) {
    if (hepd == NULL) {
        return EPD_NULL_ARG;
    }

    GFX_DrawHLine(&hepd->hgfx, hepd->gfx_buffer, x, y, len, thickness, color);

    const GFX_Rect_t hline_bounds = {
        .x = x,
        .y = y,
        .w = len,
        .h = thickness,
    };

    expand_dirty_region(hepd, hline_bounds);

    return EPD_OK;
}

EPD_Status_t EPD_DrawVLine(EPD_Handle_t *hepd, const int16_t x, const int16_t y,
                           const uint16_t len, const uint16_t thickness, const uint8_t color) {
    if (hepd == NULL) {
        return EPD_NULL_ARG;
    }

    GFX_DrawVLine(&hepd->hgfx, hepd->gfx_buffer, x, y, len, thickness, color);

    const GFX_Rect_t vline_bounds = {
        .x = x,
        .y = y,
        .w = thickness,
        .h = len,
    };

    expand_dirty_region(hepd, vline_bounds);

    return EPD_OK;
}

EPD_Status_t EPD_DrawChar(EPD_Handle_t *hepd, const int16_t x, const int16_t y,
                          const char c, const GFX_Font_t *font, const uint8_t color) {
    if (hepd == NULL || font == NULL) {
        return EPD_NULL_ARG;
    }

    GFX_DrawChar(&hepd->hgfx, hepd->gfx_buffer, x, y, c, font, color);

    GFX_Rect_t char_bounds;
    GFX_GetCharBounds(font, c, x, y, &char_bounds);

    expand_dirty_region(hepd, char_bounds);

    return EPD_OK;
}

EPD_Status_t EPD_DrawCharBg(EPD_Handle_t *hepd, const int16_t x, const int16_t y,
                            const char c, const GFX_Font_t *font, const uint8_t color,
                            const uint8_t bg) {
    if (hepd == NULL || font == NULL) {
        return EPD_NULL_ARG;
    }

    GFX_Rect_t char_bounds;
    GFX_GetCharBounds(font, c, x, y, &char_bounds);

    GFX_FillRect(&hepd->hgfx, hepd->gfx_buffer, char_bounds, bg);
    GFX_DrawChar(&hepd->hgfx, hepd->gfx_buffer, x, y, c, font, color);

    expand_dirty_region(hepd, char_bounds);

    return EPD_OK;
}

EPD_Status_t EPD_DrawText(EPD_Handle_t *hepd, const int16_t x, const int16_t y,
                          const char *text, const GFX_Font_t *font, const uint8_t color) {
    if (hepd == NULL || text == NULL || font == NULL) {
        return EPD_NULL_ARG;
    }

    GFX_DrawText(&hepd->hgfx, hepd->gfx_buffer, x, y, text, font, color);

    GFX_Rect_t text_bounds;
    GFX_GetTextBounds(font, text, x, y, &text_bounds);

    expand_dirty_region(hepd, text_bounds);

    return EPD_OK;
}

EPD_Status_t EPD_DrawTextBg(EPD_Handle_t *hepd, const int16_t x, const int16_t y,
                            const char *text, const GFX_Font_t *font, const uint8_t color,
                            const uint8_t bg) {
    if (hepd == NULL || text == NULL || font == NULL) {
        return EPD_NULL_ARG;
    }

    GFX_Rect_t text_bounds;
    GFX_GetTextBounds(font, text, x, y, &text_bounds);

    GFX_FillRect(&hepd->hgfx, hepd->gfx_buffer, text_bounds, bg);
    GFX_DrawText(&hepd->hgfx, hepd->gfx_buffer, x, y, text, font, color);

    expand_dirty_region(hepd, text_bounds);

    return EPD_OK;
}

EPD_Status_t EPD_DrawBitmap(EPD_Handle_t *hepd, const int16_t x, const int16_t y,
                            const uint8_t *bitmap, const uint16_t w, const uint16_t h,
                            const uint8_t color) {
    if (hepd == NULL || bitmap == NULL) {
        return EPD_NULL_ARG;
    }

    GFX_DrawBitmap(&hepd->hgfx, hepd->gfx_buffer, x, y, bitmap, w, h, color);

    const GFX_Rect_t bitmap_bounds = {
        .x = x,
        .y = y,
        .w = w,
        .h = h
    };

    expand_dirty_region(hepd, bitmap_bounds);

    return EPD_OK;
}

EPD_Status_t EPD_DrawBitmapBg(EPD_Handle_t *hepd, const int16_t x, const int16_t y,
                              const uint8_t *bitmap, const uint16_t w, const uint16_t h,
                              const uint8_t color, const uint8_t bg) {
    if (hepd == NULL || bitmap == NULL) {
        return EPD_NULL_ARG;
    }

    GFX_DrawBitmapBg(&hepd->hgfx, hepd->gfx_buffer, x, y, bitmap, w, h, color, bg);

    const GFX_Rect_t bitmap_bounds = {
        .x = x,
        .y = y,
        .w = w,
        .h = h
    };

    expand_dirty_region(hepd, bitmap_bounds);

    return EPD_OK;
}

EPD_Status_t EPD_FillRegion(EPD_Handle_t *hepd, const int16_t x, const int16_t y, const uint16_t w, const uint16_t h, const uint8_t color) {
    if (hepd == NULL) {
        return EPD_NULL_ARG;
    }

    const GFX_Rect_t region = {
        .x = x,
        .y = y,
        .w = w,
        .h = h
    };

    GFX_FillRect(&hepd->hgfx, hepd->gfx_buffer, region, color);

    expand_dirty_region(hepd, region);

    return EPD_OK;
}

EPD_Status_t EPD_UpdateDisplay(EPD_Handle_t *hepd) {
    if (hepd == NULL) {
        return EPD_NULL_ARG;
    }

    EPD_Status_t status;

    status = refresh_display(hepd);
    if (status != EPD_OK) {
        return status;
    }

    hepd->has_dirty_region = false;

    return EPD_OK;
}

EPD_Status_t EPD_UpdateDisplayPartial(EPD_Handle_t *hepd) {
    if (hepd == NULL) {
        return EPD_NULL_ARG;
    }

    EPD_Status_t status;

    status = update_region(hepd, hepd->dirty_region);
    if (status != EPD_OK) {
        return status;
    }

    hepd->has_dirty_region = false;

    return EPD_OK;
}

static EPD_Status_t update_region(EPD_Handle_t *hepd, GFX_Rect_t region) {
    assert_param(hepd != NULL);

    EPD_Status_t status;

    // region guard
    if (region.x + region.w > hepd->hgfx.vr_width) {
        region.w = hepd->hgfx.vr_width - region.x;
    }
    if (region.y + region.h > hepd->hgfx.vr_height) {
        region.h = hepd->hgfx.vr_height - region.y;
    }

    GFX_Rect_t raw_region;
    GFX_GetRawRegion(&hepd->hgfx, region, &raw_region);

    const uint32_t region_size = (raw_region.w / 8) * raw_region.h;

    if (region_size > EPD_PARTIAL_BUFFER_SIZE) {
        return EPD_BUFFER_OVERFLOW;
    }

    status = refresh_display_partial(hepd, region_size, raw_region);
    if (status != EPD_OK) {
        return status;
    }

    return EPD_OK;
}

static EPD_Status_t refresh_display(EPD_Handle_t *hepd) {
    assert_param(hepd != NULL);

    CHECK_STATUS_UC8253(UC8253_Init(&hepd->hdrv));
    CHECK_STATUS_UC8253(UC8253_WriteImgNew(&hepd->hdrv, hepd->frame_size, hepd->gfx_buffer));
    CHECK_STATUS_UC8253(UC8253_UpdateDisplay(&hepd->hdrv));
    CHECK_STATUS_UC8253(UC8253_EnterDeepSleep(&hepd->hdrv));

    memcpy(hepd->frame_buffer, hepd->gfx_buffer, hepd->frame_size);

    return EPD_OK;
}

static EPD_Status_t refresh_display_partial(EPD_Handle_t *hepd, const uint16_t region_size, const GFX_Rect_t region) {
    assert_param(hepd != NULL);

    CHECK_STATUS_UC8253(UC8253_Init_Partial(&hepd->hdrv));
    CHECK_STATUS_UC8253(UC8253_EnterPartialMode(&hepd->hdrv, region.x, region.y, region.w, region.h));

    GFX_ExtractRegion(hepd->frame_buffer, hepd->hgfx.config.width, region, region_buffer);
    CHECK_STATUS_UC8253(UC8253_WriteImgOld(&hepd->hdrv, region_size, region_buffer));

    GFX_ExtractRegion(hepd->gfx_buffer, hepd->hgfx.config.width, region, region_buffer);
    CHECK_STATUS_UC8253(UC8253_WriteImgNew(&hepd->hdrv, region_size, region_buffer));

    CHECK_STATUS_UC8253(UC8253_UpdateDisplay(&hepd->hdrv));
    CHECK_STATUS_UC8253(UC8253_EnterDeepSleep(&hepd->hdrv));

    GFX_CommitRegion(hepd->frame_buffer, hepd->hgfx.config.width, region, region_buffer);

    return EPD_OK;
}

static void expand_dirty_region(EPD_Handle_t *hepd, const GFX_Rect_t region) {
    assert_param(hepd != NULL);

    if (!hepd->has_dirty_region) {
        hepd->dirty_region = region;
        hepd->has_dirty_region = true;
    }

    const uint16_t x2_old = hepd->dirty_region.x + hepd->dirty_region.w;
    const uint16_t y2_old = hepd->dirty_region.y + hepd->dirty_region.h;
    const uint16_t x2_new = region.x + region.w;
    const uint16_t y2_new = region.y + region.h;

    const uint16_t x2_max = x2_old > x2_new ? x2_old : x2_new;
    const uint16_t y2_max = y2_old > y2_new ? y2_old : y2_new;

    hepd->dirty_region.x = hepd->dirty_region.x < region.x ? hepd->dirty_region.x : region.x;
    hepd->dirty_region.y = hepd->dirty_region.y < region.y ? hepd->dirty_region.y : region.y;

    hepd->dirty_region.w = x2_max - hepd->dirty_region.x;
    hepd->dirty_region.h = y2_max - hepd->dirty_region.y;
}