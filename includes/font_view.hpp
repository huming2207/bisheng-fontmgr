#pragma once

#include <cstdint>
#include <esp_err.h>
#include <esp_log.h>
#include <lvgl.h>
#include <esp_heap_caps.h>
#include <sys/unistd.h>

#include "font_tlsf.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

#include "font_disk_cacher.hpp"

#define STBTT_MEM_INCREMENT_SIZE (98304)

class font_view
{
public:
    static bool get_glyph_dsc_handler(const lv_font_t *font, lv_font_glyph_dsc_t *dsc_out, uint32_t unicode_letter, uint32_t unicode_letter_next)
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

    static const uint8_t *get_glyph_bitmap_handler(const lv_font_t *font, uint32_t unicode_letter)
    {
        if (font == nullptr || font->user_data == nullptr) {
            return nullptr;
        }

        auto *ctx = (font_view *)font->user_data;
        float scale = ctx->scale;

        ESP_LOGD(TAG, "Find glyph 0x%x, %p", unicode_letter, ctx->font_buf);

        if (ctx->disable_cache) {
            int width = 0, height = 0;
            if (stbtt_GetCodepointBitmapPtr(&ctx->stb_font, scale, scale, (int)unicode_letter, ctx->font_buf, &width, &height, nullptr, nullptr)) {
                return ctx->font_buf;
            } else {
                return nullptr;
            }
        } else {
            auto &cache = font_disk_cacher::instance();
            auto ret = cache.get_bitmap(ctx->name, ctx->height_px, unicode_letter, ctx->font_buf, (ctx->height_px * ctx->height_px), nullptr);
            int width = 0, height = 0;
            if (ret == ESP_OK) {
                ESP_LOGD(TAG, "Cache match!");
                return ctx->font_buf;
            } else if (ret == ESP_ERR_NOT_FOUND) {
                ESP_LOGD(TAG, "Cache miss!");
                if (stbtt_GetCodepointBitmapPtr(&ctx->stb_font, scale, scale, (int)unicode_letter, ctx->font_buf, &width, &height, nullptr, nullptr)) {
                    cache.add_bitmap(ctx->name, ctx->height_px, unicode_letter, ctx->font_buf, (ctx->height_px * ctx->height_px));
                    ESP_LOGD(TAG, "Cache added!");
                    return ctx->font_buf;
                } else {
                    ESP_LOGD(TAG, "Codepoint not found!");
                    return nullptr;
                }
            } else {
                ESP_LOGD(TAG, "Something else screwed up");
                return nullptr;
            }
        }
    }

    static void *stbtt_mem_alloc(size_t len, void *_ctx)
    {
        auto *ctx = (font_view *)(_ctx);
        if (!ctx->tlsf_inited || ctx->tlsf == nullptr) {
            ctx->tlsf = stbtt_tlsf_create_with_pool(ctx->mem_pool, STBTT_MEM_INCREMENT_SIZE);
            if (ctx->tlsf == nullptr) {
                ESP_LOGE(TAG, "Heap pool add fail");
                return nullptr;
            } else {
                ctx->tlsf_inited = true;
            }
        }

        auto *ret = stbtt_tlsf_malloc(ctx->tlsf, len);
        if (ret != nullptr) ctx->used_mem += stbtt_tlsf_block_size(ret);
        ESP_LOGD(TAG, "alloc: +%u; %u", len, ctx->used_mem);
        return ret;
    }

    static void stbtt_mem_free(void *ptr, void *_ctx)
    {
        auto *ctx = (font_view *)(_ctx);
        auto size = stbtt_tlsf_block_size(ptr);
        ctx->used_mem -= size;
        ESP_LOGD(TAG, "free: -%u; %u", size, ctx->used_mem);
        stbtt_tlsf_free(ctx->tlsf, ptr);

        if (ctx->used_mem < 1) {
            stbtt_tlsf_destroy(ctx->tlsf);
            ctx->tlsf_inited = false;
        }
    }

public:
    explicit font_view(const char *_name, bool _disable_cache = true)
    {
        name = strdup(_name);
        disable_cache = _disable_cache;
    }

    ~font_view()
    {
        if (name != nullptr) {
            free((void *)name);
        }

        if (font_buf != nullptr) {
            free((void *) font_buf);
        }

        if (tlsf != nullptr) {
            stbtt_tlsf_destroy(tlsf);
        }

        if (mem_pool != nullptr) {
            free((void *) mem_pool);
        }

        if (ttf_buf != nullptr) {
            free(ttf_buf);
        }
    }

    esp_err_t init(const char *file_path, uint8_t _height_px)
    {
        if (file_path == nullptr || _height_px < 1) {
            return ESP_ERR_INVALID_ARG;
        }

        if (access(file_path, F_OK) != 0) {
            ESP_LOGE(TAG, "Font %s not found", file_path);
            return ESP_ERR_NOT_FOUND;
        }

        FILE *fp = fopen(file_path, "rb");
        if (fp == nullptr) {
            ESP_LOGE(TAG, "Failed to open font %s", file_path);
            return ESP_ERR_INVALID_STATE;
        }

        fseek(fp, 0, SEEK_END);
        size_t len = ftell(fp);
        if (len < 1) {
            ESP_LOGE(TAG, "Invalid length for font %s; %zu", file_path, len);
            fclose(fp);
            return ESP_ERR_INVALID_SIZE;
        }

        ttf_buf = static_cast<uint8_t *>(heap_caps_malloc_prefer(len, MALLOC_CAP_SPIRAM, MALLOC_CAP_INTERNAL));
        if (ttf_buf == nullptr) {
            ESP_LOGE(TAG, "Failed to alloc TTF buf");
            fclose(fp);
            return ESP_ERR_NO_MEM;
        }

        rewind(fp);
        if (fread(ttf_buf, 1, len, fp) != len) {
            ESP_LOGE(TAG, "TTF read length mismatch");
            fclose(fp);
            free(ttf_buf);
            return ESP_ERR_NO_MEM;
        }

        fclose(fp);
        return init(ttf_buf, len, _height_px);
    }

    esp_err_t init(const uint8_t *buf, size_t len, uint8_t _height_px)
    {
        height_px = _height_px;
        ttf_len = len;
        lv_font.line_height = height_px;
        lv_font.get_glyph_dsc = get_glyph_dsc_handler;
        lv_font.get_glyph_bitmap = get_glyph_bitmap_handler;
        lv_font.user_data = this;
        lv_font.base_line = 0;
        lv_font.subpx = LV_FONT_SUBPX_NONE;

        stb_font.userdata = this;
        stb_font.heap_alloc_func = stbtt_mem_alloc;
        stb_font.heap_free_func = stbtt_mem_free;

        if (stbtt_InitFont(&stb_font, buf, 0) < 1) {
            ESP_LOGE(TAG, "Load font failed");
            return ESP_FAIL;
        }

        if (!disable_cache) {
            auto &cache = font_disk_cacher::instance();
            auto ret = cache.add_renderer(name, _height_px);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Failed to create renderer cache namespace");
                return ret;
            }
        }

        ESP_LOGD(TAG, "Allocating font buffer size %u bytes", _height_px * _height_px);

        font_buf = (uint8_t *)heap_caps_malloc(_height_px * _height_px, MALLOC_CAP_SPIRAM);
        if (font_buf == nullptr) {
            ESP_LOGE(TAG, "Failed to allocate font buffer");
            return ESP_ERR_NO_MEM;
        }

        memset(font_buf, 0, _height_px * _height_px);

        mem_pool = (uint8_t *)heap_caps_aligned_calloc(4, STBTT_MEM_INCREMENT_SIZE, 1, MALLOC_CAP_SPIRAM);
        if (mem_pool == nullptr) {
            ESP_LOGE(TAG, "Failed to allocate heap buffer");
            return ESP_ERR_NO_MEM;
        } else {
            ESP_LOGI(TAG, "Alloc size %zu, %zu, %zu", stbtt_tlsf_block_size_max(), stbtt_tlsf_size(), stbtt_tlsf_pool_overhead());
            tlsf = stbtt_tlsf_create_with_pool(mem_pool, STBTT_MEM_INCREMENT_SIZE);
            tlsf_inited = true;
            if (tlsf == nullptr) {
                ESP_LOGE(TAG, "Heap pool add fail");
                return ESP_ERR_NO_MEM;
            }
        }

        scale = stbtt_ScaleForPixelHeight(&stb_font, height_px);
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

    esp_err_t decorate_font_obj(lv_obj_t *obj)
    {
        if (obj == nullptr) {
            return ESP_ERR_INVALID_ARG;
        }

        lv_obj_set_style_text_font(obj, &lv_font, 0);
        return ESP_OK;
    }

private:
    volatile bool tlsf_inited = false;
    uint8_t height_px = 0;
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
    stbtt_tlsf_t tlsf = nullptr;
};
