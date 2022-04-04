#pragma once

#include <cstdint>
#include <esp_err.h>
#include <esp_log.h>
#include <lvgl.h>

#include <schrift.h>


class font_view
{
public:
    static bool glyph_dsc_handler(const lv_font_t *font, lv_font_glyph_dsc_t *dsc_out, uint32_t unicode_letter, uint32_t unicode_letter_next)
    {
        if (font == nullptr || font->user_data == nullptr) {
            return false;
        }

        auto *ctx = (font_view *)font->user_data;
        if (dsc_out == nullptr) return false;

        SFT_Glyph glyph = {};
        if (sft_lookup(&ctx->sft, unicode_letter, &glyph) < 0) {
            ESP_LOGW(TAG, "Missing glyph: 0x%x", unicode_letter);
            return false;
        }

        SFT_GMetrics g_mtx = {};
        if (sft_gmetrics(&ctx->sft, glyph, &g_mtx)) {
            ESP_LOGW(TAG, "Missing metrics: 0x%x", unicode_letter);
            return false;
        }

        dsc_out->box_w = g_mtx.minWidth;
        dsc_out->box_h = g_mtx.minHeight;
        dsc_out->ofs_x = (int16_t)(g_mtx.leftSideBearing);
        dsc_out->ofs_y = 0;
        dsc_out->adv_w = 0; // TODO: fix this later
        dsc_out->bpp = 8;

        ESP_LOGI(TAG, "box_w: %u, box_h: %u, off_x: %u, off_y: %u", dsc_out->box_w, dsc_out->box_h, dsc_out->ofs_x, dsc_out->ofs_y);

        return true;
    }

    static const uint8_t *glyph_bitmap_handler(const lv_font_t *font, uint32_t unicode_letter)
    {
        if (font == nullptr || font->user_data == nullptr) {
            return nullptr;
        }

        auto *ctx = (font_view *)font->user_data;

        SFT_Glyph glyph = {};
        if (sft_lookup(&ctx->sft, unicode_letter, &glyph) < 0) {
            ESP_LOGW(TAG, "Missing glyph: 0x%x", unicode_letter);
            return nullptr;
        }

        SFT_GMetrics g_mtx = {};
        if (sft_gmetrics(&ctx->sft, glyph, &g_mtx)) {
            ESP_LOGW(TAG, "Missing metrics: 0x%x", unicode_letter);
            return nullptr;
        }

        SFT_Image img = {};
        img.width  = (g_mtx.minWidth + 3) & ~3;
        img.height = g_mtx.minHeight;
        img.pixels = ctx->font_buf;

        if (sft_render(&ctx->sft, glyph, img) < 0) {
            ESP_LOGW(TAG, "Failed when render: 0x%x", unicode_letter);
            return nullptr;
        }

        return ctx->font_buf;
    }

public:
    esp_err_t init(const uint8_t *buf, size_t len, uint8_t _height_px)
    {
        height_px = _height_px;
        lv_font.line_height = height_px;
        lv_font.get_glyph_dsc = glyph_dsc_handler;
        lv_font.get_glyph_bitmap = glyph_bitmap_handler;
        lv_font.user_data = this;
        lv_font.base_line = 0;
        lv_font.subpx = LV_FONT_SUBPX_NONE; // This has to be NONE otherwise it will show nothing...

        font_buf = (uint8_t *)heap_caps_malloc(_height_px * _height_px, MALLOC_CAP_SPIRAM);
        if (font_buf == nullptr) {
            ESP_LOGE(TAG, "Failed to allocate font buffer");
            return ESP_ERR_NO_MEM;
        }

        memset(font_buf, 0, _height_px * _height_px);

        sft.xScale = _height_px;
        sft.yScale = _height_px;
        sft.flags  = SFT_DOWNWARD_Y;
        sft.font = sft_loadmem(buf, len);
        if (sft.font == nullptr) {
            ESP_LOGE(TAG, "Failed to load font");
            free(font_buf);
            return ESP_ERR_NO_MEM;
        }

        if (sft_lmetrics(&sft, &l_mtx) < 0) {
            ESP_LOGE(TAG, "Failed to read font metadata (hhea table)");
            free(font_buf);
            sft_freefont(sft.font);
            return ESP_ERR_NO_MEM;
        }

        return ESP_OK;
    }

    esp_err_t decorate_font_style(lv_style_t *style)
    {
        if (style == nullptr) {
            return ESP_ERR_INVALID_ARG;
        }

        lv_style_set_text_font(style, &lv_font);
        return ESP_OK;
    }


private:
    uint8_t height_px = 0;
    uint8_t *font_buf = nullptr;
    lv_font_t lv_font = {};
    SFT sft = {};
    SFT_LMetrics l_mtx = {};
    static const constexpr char *TAG = "font_view";
};