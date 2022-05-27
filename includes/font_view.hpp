#pragma once

#include <cstdint>
#include <esp_err.h>
#include <esp_log.h>
#include <lvgl.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

#include "font_view.hpp"

class font_view
{
public:
    static bool get_glyph_dsc_handler(const lv_font_t *font, lv_font_glyph_dsc_t *dsc_out, uint32_t unicode_letter, uint32_t unicode_letter_next);
    static const uint8_t *get_glyph_bitmap_handler(const lv_font_t *font, uint32_t unicode_letter);

public:
    esp_err_t init(const uint8_t *buf, size_t len, uint8_t _height_px, bool disable_cache);
    esp_err_t decorate_font_style(lv_style_t *style);

private:
    uint8_t height_px = 0;
    uint8_t *font_buf = nullptr;
    uint32_t renderer_id = 0;
    float scale = 0;
    lv_font_t lv_font = {};
    stbtt_fontinfo stb_font = {};
    static const constexpr char *TAG = "font_view";
};
