#pragma once

#include <esp_err.h>
#include <esp_log.h>
#include <lvgl.h>
#include <esp_heap_caps.h>
#include <sys/unistd.h>

#include <stb_truetype.h>

#include "font_disk_cacher.hpp"

class font_view
{
public:
    static bool get_glyph_dsc_handler(const lv_font_t *font, lv_font_glyph_dsc_t *dsc_out, uint32_t unicode_letter, uint32_t unicode_letter_next);
    static const uint8_t *get_glyph_bitmap_handler(const lv_font_t *font, uint32_t unicode_letter);
    static void *stbtt_mem_alloc(size_t len, void *_ctx);
    static void stbtt_mem_free(void *ptr, void *_ctx);

public:
    explicit font_view(const char *_name, bool _disable_cache = true);
    ~font_view();

    esp_err_t decorate_font_style(lv_style_t *style);
    esp_err_t decorate_font_obj(lv_obj_t *obj);
    esp_err_t init(const char *file_path, uint8_t _height_px);
    esp_err_t init(const uint8_t *buf, size_t len, uint8_t _height_px);

private:
    uint8_t height_px = 0;
    volatile bool tlsf_inited = false;
    bool disable_cache = false;

    uint8_t *ttf_buf = nullptr;
    size_t ttf_len = 0;
    uint8_t *font_buf = nullptr;
    uint8_t *mem_pool = nullptr;
    const char *name = nullptr;
    uint32_t used_mem = 0;

    float scale = 0;

    lv_font_t lv_font = {};
    stbtt_fontinfo stb_font = {};
    static const constexpr char *TAG = "font_view";
    multi_heap_handle_t heap = nullptr;
};
