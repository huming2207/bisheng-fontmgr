#include <esp_heap_caps.h>
#include <esp_log.h>
#include <esp_timer.h>

#include "font_cacher.hpp"

esp_err_t font_cacher::init(size_t buf_size, size_t glyph_cnt)
{
    if (cache_size != 0 || cached_glyphs != nullptr) {
        ESP_LOGE(TAG, "Font cacher already initialised");
        return ESP_ERR_INVALID_STATE;
    }

    if (buf_size < 1 || glyph_cnt < 1) {
        ESP_LOGE(TAG, "Invalid size arg");
        return ESP_ERR_INVALID_ARG;
    }

#ifdef CONFIG_SPIRAM
    cached_glyphs = (glyph_item *)heap_caps_calloc(glyph_cnt, sizeof(glyph_item), MALLOC_CAP_SPIRAM);
#else
    cached_glyphs = (glyph_item *)calloc(glyph_cnt, sizeof(glyph_item));
#endif

    if (cached_glyphs == nullptr) {
        ESP_LOGE(TAG, "No mem");
        return ESP_ERR_NO_MEM;
    }

    cache_size = buf_size;
    glyph_slot_size = glyph_cnt;
    cache_used = 0;

    return ESP_OK;
}

uint32_t font_cacher::reg_instance()
{
    if (unlikely(instance_ctr == UINT32_MAX)) {
        instance_ctr = 0;
    }

    return instance_ctr++;
}

bool font_cacher::has_cache(uint32_t renderer_id, uint32_t codepoint)
{
    for (size_t idx = 0; idx < glyph_slot_size; idx += 1) {
        if (cached_glyphs[idx].codepoint == codepoint && cached_glyphs[idx].renderer_instance_id == renderer_id) {
            return true;
        }
    }

    return false;
}

esp_err_t font_cacher::get_cache(uint32_t renderer_id, uint32_t codepoint, glyph_item *out)
{
    if (out == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    for (size_t idx = 0; idx < glyph_slot_size; idx += 1) {
        if (cached_glyphs[idx].codepoint == codepoint && cached_glyphs[idx].renderer_instance_id == renderer_id) {
            out->bitmap = cached_glyphs[idx].bitmap;
            out->last_used = cached_glyphs[idx].last_used;
            out->renderer_instance_id = cached_glyphs[idx].renderer_instance_id;
            out->codepoint = cached_glyphs[idx].codepoint;
            out->len = cached_glyphs[idx].len;

            return ESP_OK;
        }
    }

    return ESP_ERR_NOT_FOUND;
}

esp_err_t font_cacher::add_cache(uint32_t renderer_id, uint32_t codepoint, uint8_t *buf_in, size_t buf_sz)
{
    size_t idx = 0;

    auto ret = make_room(&idx, buf_sz);
    if (ret != ESP_OK) {
        return ret;
    }

    cached_glyphs[idx].last_used = esp_timer_get_time();
    cached_glyphs[idx].bitmap = buf_in;
    cached_glyphs[idx].codepoint = codepoint;
    cached_glyphs[idx].len = buf_sz;
    cached_glyphs[idx].renderer_instance_id = renderer_id;

    cache_used += buf_sz;
    glyph_slot_cnt += 1;

    return ESP_OK;
}

esp_err_t font_cacher::make_room(size_t *free_idx, size_t space_needed)
{
    // Find the oldest
    glyph_item *oldest_item = nullptr;
    uint64_t oldest_ts = UINT64_MAX;
    size_t pending_idx = 0;

    // Loop to clear out enough space as per requested
    do {
        for (size_t idx = 0; idx < glyph_slot_size; idx += 1) {
            if (cached_glyphs[idx].bitmap == nullptr && cached_glyphs[idx].last_used == 0) {
                if (free_idx != nullptr) {
                    *free_idx = idx;
                }

                if (cache_size - cache_used >= space_needed) {
                    return ESP_OK;
                }
            }

            if (cached_glyphs[idx].last_used < oldest_ts) {
                oldest_ts = cached_glyphs[idx].last_used;
                oldest_item = &cached_glyphs[idx];
                pending_idx = idx;
            }
        }

        if (oldest_item == nullptr) {
            ESP_LOGE(TAG, "Failed to make room (mem corrupt)");
            return ESP_ERR_NO_MEM;
        } else {
            cache_used -= oldest_item->len;
            glyph_slot_cnt -= 1;

            oldest_item->last_used = 0;
            oldest_item->renderer_instance_id = 0;
            oldest_item->len = 0;
            oldest_item->codepoint = 0;
            if (oldest_item->bitmap != nullptr) {
                free(oldest_item->bitmap);
                oldest_item->bitmap = nullptr;
            }

            if (free_idx != nullptr) {
                *free_idx = pending_idx;
            }
        }
    } while (cache_size - cache_used < space_needed);

    return ESP_OK;
}
