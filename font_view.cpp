#include <cmath>
#include <esp_heap_caps.h>
#include "font_view.hpp"
#include "font_cacher.hpp"

bool font_view::get_glyph_dsc_handler(const lv_font_t *font, lv_font_glyph_dsc_t *dsc_out, uint32_t unicode_letter, uint32_t unicode_letter_next)
{
    if (font == nullptr || font->user_data == nullptr) {
        return false;
    }

    auto *ctx = (font_view *)font->user_data;

    if (dsc_out == nullptr) return false;
    float scale = ctx->scale;

    if (stbtt_FindGlyphIndex(&ctx->stb_font, (int)unicode_letter) == 0) {
        return false;
    }

    int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
    stbtt_GetCodepointBitmapBox(&ctx->stb_font, (int)unicode_letter, scale, scale, &x0, &y0, &x1, &y1);

    int adv_w = 0;
    int left_side_bearing = 0;
    stbtt_GetCodepointHMetrics(&ctx->stb_font, (int)unicode_letter, &adv_w, &left_side_bearing);

    int kern = stbtt_GetCodepointKernAdvance(&ctx->stb_font, (int)unicode_letter, (int)unicode_letter_next);
    adv_w = (int)(roundf((float)adv_w * scale));
    kern = (int)(roundf((float)kern * scale));
    left_side_bearing = (int)(roundf(((float)left_side_bearing * scale)));

    dsc_out->adv_w = adv_w + kern;
    dsc_out->box_h = (uint16_t)(y1 - y0);
    dsc_out->box_w = (uint16_t)(x1 - x0);
    dsc_out->ofs_x = (int16_t)left_side_bearing;
    dsc_out->ofs_y = (int16_t)(y1 * -1);
    dsc_out->bpp = 8;
    return true;
}

const uint8_t *font_view::get_glyph_bitmap_handler(const lv_font_t *font, uint32_t unicode_letter)
{
    if (font == nullptr || font->user_data == nullptr) {
        return nullptr;
    }

    auto *ctx = (font_view *)font->user_data;
    float scale = ctx->scale;
    auto &cache = font_cacher::instance();

    if (ctx->font_buf == nullptr) {
        glyph_item glyph{};
        auto ret = cache.get_cache(ctx->renderer_id, unicode_letter, &glyph);
        if (ret == ESP_OK) {
            return glyph.bitmap;
        } else if (ret == ESP_ERR_NOT_FOUND) {
            int width = 0, height = 0;
            auto *buf = stbtt_GetCodepointBitmap(&ctx->stb_font, scale, scale, (int)unicode_letter, &width, &height, nullptr, nullptr);
            if (buf != nullptr) {
                cache.add_cache(ctx->renderer_id, unicode_letter, buf, ctx->height_px * ctx->height_px);
            }

            return buf;
        } else {
            return nullptr;
        }
    } else {
        int width = 0, height = 0;
        if (stbtt_GetCodepointBitmapPtr(&ctx->stb_font, scale, scale, (int)unicode_letter, ctx->font_buf, &width, &height, nullptr, nullptr)) {
            return ctx->font_buf;
        } else {
            return nullptr;
        }
    }
}

esp_err_t font_view::init(const uint8_t *buf, size_t len, uint8_t _height_px, bool disable_cache)
{
    height_px = _height_px;
    lv_font.line_height = height_px;
    lv_font.get_glyph_dsc = get_glyph_dsc_handler;
    lv_font.get_glyph_bitmap = get_glyph_bitmap_handler;
    lv_font.user_data = this;
    lv_font.base_line = 0;
    lv_font.subpx = LV_FONT_SUBPX_NONE;

    auto &cache = font_cacher::instance();
    renderer_id = cache.get_new_renderer_id();

    if (stbtt_InitFont(&stb_font, buf, 0) < 1) {
        ESP_LOGE(TAG, "Load font failed");
        return ESP_FAIL;
    }

    if (disable_cache) {
        ESP_LOGD(TAG, "Allocating font buffer size %u bytes", _height_px * _height_px);

        font_buf = (uint8_t *)heap_caps_malloc(_height_px * _height_px, MALLOC_CAP_SPIRAM);
        if (font_buf == nullptr) {
            ESP_LOGE(TAG, "Failed to allocate font buffer");
            return ESP_ERR_NO_MEM;
        }

        memset(font_buf, 0, _height_px * _height_px);
    } else {
        font_buf = nullptr;
    }

    scale = stbtt_ScaleForPixelHeight(&stb_font, height_px);
    return ESP_OK;
}

esp_err_t font_view::decorate_font_style(lv_style_t *style)
{
    if (style == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    lv_style_set_text_font(style, &lv_font);
    return ESP_OK;
}
