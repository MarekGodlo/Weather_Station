/**
 * @file gfx.c
 *
 * Created on 11.07.2026.
 * @author Marek Godlowicz https://github.com/MarekGodlo
 * @brief Core graphics library providing common drawing primitives
 *        (points, lines, bitmap, etc.), ported to plain C.
 *        Based on the Adafruit GFX Library.
 */

#include "gfx.h"

#include <string.h>

static uint16_t uint16_min(uint16_t a, uint16_t b);

void GFX_Init(GFX_Handle_t *hgfx, const GFX_Config_t *config) {
    assert_param(hgfx != NULL);
    assert_param(config != NULL);

    if (hgfx == NULL || config == NULL) {
        return;
    }

    hgfx->config = *config;

    hgfx->vr_height = hgfx->config.height;
    hgfx->vr_width = hgfx->config.width;
    hgfx->rotation = GFX_ROTATION_0;
}

void GFX_SetRotation(GFX_Handle_t *hgfx, const GFX_Rotation_t rotation) {
    assert_param(hgfx != NULL);

    hgfx->rotation = rotation;

    switch (hgfx->rotation) {
        case GFX_ROTATION_0:
        case GFX_ROTATION_180:
            hgfx->vr_width = hgfx->config.width;
            hgfx->vr_height = hgfx->config.height;
            break;
        case GFX_ROTATION_90:
        case GFX_ROTATION_270:
            hgfx->vr_width = hgfx->config.height;
            hgfx->vr_height = hgfx->config.width;
            break;
    }
}

void GFX_DrawPixel(const GFX_Handle_t *hgfx, uint8_t *gfx_buffer, int16_t x, int16_t y, const uint8_t color) {
    assert_param(hgfx != NULL);
    assert_param(gfx_buffer != NULL);

    if(gfx_buffer) {
        if((x < 0) || (y < 0) || (x >= hgfx->vr_width) || (y >= hgfx->vr_height)) {
            return;
        }

        int16_t t;
        switch (hgfx->rotation) {
            case GFX_ROTATION_90:
                t = x;
                x = hgfx->config.width - 1 - y;
                y = t;
                break;
            case GFX_ROTATION_180:
                x = hgfx->config.width - 1 - x;
                y = hgfx->config.height - 1 - y;
                break;
            case GFX_ROTATION_270:
                t = x;
                x = y;
                y = hgfx->config.height - 1 - t;
                break;
        }

        uint8_t *ptr  = &gfx_buffer[(x / 8) + y * ((hgfx->config.width + 7) / 8)];

        if(color) {
            *ptr |=   0x80 >> (x & 7);
        } else {
            *ptr &= ~(0x80 >> (x & 7));
        }
    }
}

void GFX_DrawHLine(const GFX_Handle_t *hgfx, uint8_t *gfx_buffer,
                   const int16_t x, const int16_t y, const uint16_t len,
                   const uint16_t thickness, const uint8_t color) {
    assert_param(hgfx != NULL);
    assert_param(gfx_buffer != NULL);

    for (int16_t t = 0; t < thickness; ++t) {
        for (int16_t i = 0; i < len; ++i) {
            GFX_DrawPixel(hgfx, gfx_buffer, x + i, y + t, color);
        }
    }
}

void GFX_DrawVLine(const GFX_Handle_t *hgfx, uint8_t *gfx_buffer,
                   const int16_t x, const int16_t y, const uint16_t len,
                   const uint16_t thickness, const uint8_t color) {
    assert_param(hgfx != NULL);
    assert_param(gfx_buffer != NULL);

    for (int16_t t = 0; t < thickness; ++t) {
        for (int16_t i = 0; i < len; ++i) {
            GFX_DrawPixel(hgfx, gfx_buffer, x + t, y + i, color);
        }
    }

}

void GFX_DrawChar(const GFX_Handle_t *hgfx, uint8_t *gfx_buffer,
                  const int16_t x, const int16_t y, const uint8_t c,
                  const GFX_Font_t *font, const uint8_t color) {
    assert_param(hgfx != NULL);
    assert_param(gfx_buffer != NULL);
    assert_param(font != NULL);

    if (c < font->first || c > font->last) return;

    const GFX_Glyph_t *glyph = &font->glyph[c - font->first];
    const uint8_t *bitmap = font->bitmap;

    uint16_t bo = glyph->bitmap_offset;
    const uint8_t h = glyph->height;
    const uint8_t w = glyph->width;
    const int8_t xo = glyph->x_offset;
    const int8_t yo = glyph->y_offset;

    uint8_t bit = 0;
    uint8_t bits = 0;

    for (uint8_t yy = 0; yy < h; ++yy) {
        for (uint8_t xx = 0; xx < w; ++xx) {
            if (!(bit++ & 7)) {
                bits = bitmap[bo++];
            }

            if (bits & 0x80) {
                GFX_DrawPixel(hgfx, gfx_buffer, x + xx + xo, y + yy + yo, color);
            }
            bits <<= 1;
        }
    }
}

void GFX_DrawText(const GFX_Handle_t *hgfx, uint8_t *gfx_buffer,
                  const int16_t x, const int16_t y, const char *text,
                  const GFX_Font_t *font, const uint8_t color) {
    assert_param(hgfx != NULL);
    assert_param(gfx_buffer != NULL);
    assert_param(text != NULL);
    assert_param(font != NULL);

    int16_t cursor_x = x;
    int16_t cursor_y = y;

    while (*text) {
        char c = *text++;

        if (c == '\n') {
            cursor_x = x;
            cursor_y += font->y_advance;
        }

        if (c < font->first || c > font->last) continue;

        GFX_Glyph_t *glyph = &font->glyph[c - font->first];

        GFX_DrawChar(hgfx, gfx_buffer, cursor_x, cursor_y, c, font, color);
        cursor_x += glyph->x_advance;
    }
}

void GFX_GetCharBounds(const GFX_Font_t *font, const char c,
                        const int16_t cursor_x, const int16_t cursor_y,
                        GFX_Rect_t *out_rect) {
    assert_param(font != NULL);
    assert_param(out_rect != NULL);

    if (c < font->first || c > font->last) {
        out_rect->x = cursor_x;
        out_rect->y = cursor_y;
        out_rect->w = 0;
        out_rect->h = 0;
        return;
    }

    const GFX_Glyph_t *glyph = &font->glyph[c - font->first];

    out_rect->x = cursor_x + glyph->x_offset;
    out_rect->y = cursor_y + glyph->y_offset;
    out_rect->w = glyph->width;
    out_rect->h = glyph->height;
}

void GFX_GetTextBounds(const GFX_Font_t *font, const char *text,
                       const int16_t cursor_x, const int16_t cursor_y,
                       GFX_Rect_t *out_rect) {
    assert_param(font != NULL);
    assert_param(text != NULL);
    assert_param(out_rect != NULL);

    const char *c = text;

    int16_t x = cursor_x;
    int16_t y = cursor_y;

    int16_t max_x = INT16_MIN;
    int16_t max_y = INT16_MIN;
    int16_t min_x = INT16_MAX;
    int16_t min_y = INT16_MAX;

    GFX_Rect_t char_bounds;

    while (*c) {
        if (*c == '\n') {
            x = cursor_x;
            y += font->y_advance;
            c++;
            continue;
        }
        if (*c == '\r') {
            c++; continue;
        }

        GFX_GetCharBounds(font, *c, x, y, &char_bounds);

        if (char_bounds.w > 0 && char_bounds.h > 0) {
            int16_t x2 = char_bounds.x + char_bounds.w - 1;
            int16_t y2 = char_bounds.y + char_bounds.h - 1;

            if (char_bounds.x < min_x) min_x = char_bounds.x;
            if (char_bounds.y < min_y) min_y = char_bounds.y;
            if (x2 > max_x) max_x = x2;
            if (y2 > max_y) max_y = y2;
        }

        if (*c >= font->first && *c <= font->last) {
            x += font->glyph[*c - font->first].x_advance;
        }

        c++;
    }

    if (min_x > max_x) {
        out_rect->x = cursor_x;
        out_rect->y = cursor_y;
        out_rect->w = 0;
        out_rect->h = 0;
        return;
    }

    out_rect->x = min_x;
    out_rect->y = min_y;
    out_rect->w = (uint16_t) (max_x - min_x) + 1;
    out_rect->h = (uint16_t) (max_y - min_y) + 1;
}

void GFX_DrawBitmap(const GFX_Handle_t *hgfx, uint8_t *gfx_buffer, const int16_t x, int16_t y,
                    const uint8_t *bitmap, const uint16_t w, const uint16_t h, const uint8_t color) {
    assert_param(hgfx != NULL);
    assert_param(gfx_buffer != NULL);
    assert_param(bitmap != NULL);

    const uint16_t byteWidth = (w + 7) / 8; // Bitmap scanline pad = whole byte
    uint8_t byte = 0;

    for(uint16_t j=0; j<h; j++, y++) {
        for(uint16_t i=0; i<w; i++) {
            if(i & 7) {
                byte <<= 1;
            } else {
                byte = bitmap[j * byteWidth + i / 8];
            }

            if(byte & 0x80) {
                GFX_DrawPixel(hgfx, gfx_buffer, x+i, y, color);
            }
        }
    }
}

void GFX_DrawBitmapBg(const GFX_Handle_t *hgfx, uint8_t *gfx_buffer, const int16_t x, int16_t y,
                      const uint8_t *bitmap, const uint16_t w, const uint16_t h,
                      const uint8_t color, const uint8_t bg) {
    assert_param(hgfx != NULL);
    assert_param(gfx_buffer != NULL);
    assert_param(bitmap != NULL);

    const int16_t byteWidth = (w + 7) / 8; // Bitmap scanline pad = whole byte
    uint8_t byte = 0;

    for(int16_t j=0; j<h; j++, y++) {
        for(int16_t i=0; i<w; i++ ) {
            if(i & 7) {
                byte <<= 1;
            } else {
                byte   = bitmap[j * byteWidth + i / 8];
            }

            GFX_DrawPixel(hgfx, gfx_buffer, x+i, y, (byte & 0x80) ? color : bg);
        }
    }
}

void GFX_FillRect(const GFX_Handle_t *hgfx, uint8_t *gfx_buffer, const GFX_Rect_t rect, const uint8_t color) {
    assert_param(hgfx != NULL);
    assert_param(gfx_buffer != NULL);

    for (uint16_t row = 0; row < rect.h; ++row) {
        for (uint16_t col = 0; col < rect.w; ++col) {
            GFX_DrawPixel(hgfx, gfx_buffer, rect.x + col, rect.y + row, color);
        }
    }
}

void GFX_ExtractRegion(const uint8_t *src, const uint16_t full_width,
                       const GFX_Rect_t region, uint8_t *dst) {
    assert_param(src != NULL);
    assert_param(dst != NULL);

    const uint16_t src_stride = (full_width + 7) / 8;
    const uint16_t dst_stride = region.w / 8;

    for (uint16_t row = 0; row < region.h; row++) {
        const uint16_t current_y = region.y + row;
        const uint16_t x_byte_offset = region.x / 8;

        if (x_byte_offset < src_stride) {
            const uint8_t *src_row = &src[x_byte_offset + current_y * src_stride];
            uint8_t *dst_row = &dst[row * dst_stride];

            uint16_t bytes_to_copy = dst_stride;
            if (x_byte_offset + bytes_to_copy > src_stride) {
                bytes_to_copy = src_stride - x_byte_offset;
            }

            memcpy(dst_row, src_row, bytes_to_copy);
        }
    }
}

void GFX_CommitRegion(uint8_t *dst_full, const uint16_t full_width,
                      const GFX_Rect_t region, const uint8_t *src_region) {
    assert_param(dst_full != NULL);
    assert_param(src_region != NULL);

    const uint16_t stride = (full_width + 7) / 8;
    const uint16_t region_stride = region.w / 8;

    for (uint16_t row = 0; row < region.h; row++) {
        const uint16_t current_y = region.y + row;
        const uint16_t x_byte_offset = region.x / 8;

        if (x_byte_offset < stride) {
            uint8_t *dst_row = &dst_full[x_byte_offset + current_y * stride];
            const uint8_t *src_row = &src_region[row * region_stride];

            uint16_t bytes_to_copy = region_stride;
            if (x_byte_offset + bytes_to_copy > stride) {
                bytes_to_copy = stride - x_byte_offset;
            }

            memcpy(dst_row, src_row, bytes_to_copy);
        }
    }
}

void GFX_GetRawRegion(const GFX_Handle_t *hgfx, GFX_Rect_t region, GFX_Rect_t *raw_region) {
    assert_param(hgfx != NULL);
    assert_param(raw_region != NULL);

    region.x = uint16_min(region.x, hgfx->vr_width);
    region.y = uint16_min(region.y, hgfx->vr_height);
    region.w = uint16_min(region.w, hgfx->vr_width - region.x);
    region.h = uint16_min(region.h, hgfx->vr_height - region.y);

    switch (hgfx->rotation)
    {
        case GFX_ROTATION_0:
            *raw_region = region;
            break;
        case GFX_ROTATION_90:
            raw_region->x = hgfx->config.width - region.y - region.h;
            raw_region->y = region.x;
            raw_region->w = region.h;
            raw_region->h = region.w;
            break;
        case GFX_ROTATION_180:
            raw_region->x = hgfx->config.width  - region.x - region.w;
            raw_region->y = hgfx->config.height - region.y - region.h;
            raw_region->w = region.w;
            raw_region->h = region.h;
            break;
        case GFX_ROTATION_270:
            raw_region->x = region.y;
            raw_region->y = hgfx->config.height - region.x - region.w;
            raw_region->w = region.h;
            raw_region->h = region.w;
            break;
        default:
            *raw_region = region;
            break;
    }

    const uint16_t old_x = raw_region->x;
    raw_region->x &= ~7;
    raw_region->w = (raw_region->w + (old_x - raw_region->x) + 7) & ~7;
}

static uint16_t uint16_min(const uint16_t a, const uint16_t b) {
    return (a < b ? a : b);
}