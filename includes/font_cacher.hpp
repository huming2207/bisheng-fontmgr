#pragma once

#include <esp_err.h>

struct glyph_item
{
    uint32_t renderer_instance_id;
    uint32_t codepoint;
    uint8_t *bitmap;
    size_t len;
    uint64_t last_used;
};

class font_cacher
{
public:
    static font_cacher& instance()
    {
        static font_cacher cacher;
        return cacher;
    }

    void operator=(font_cacher const&) = delete;
    font_cacher(font_cacher const&) = delete;

private:
    font_cacher() = default;
    uint32_t instance_ctr = 0;
    size_t cache_size = 0;
    size_t cache_used = 0;
    size_t glyph_slot_size = 0;
    size_t glyph_slot_cnt = 0;
    glyph_item *cached_glyphs = nullptr;
    static constexpr const char *TAG = "ft_cacher";

public:
    esp_err_t init(size_t buf_size, size_t glyph_cnt);
    uint32_t get_new_renderer_id();
    bool has_cache(uint32_t renderer_id, uint32_t codepoint);
    esp_err_t get_cache(uint32_t renderer_id, uint32_t codepoint, glyph_item *out);
    esp_err_t add_cache(uint32_t renderer_id, uint32_t codepoint, uint8_t *buf_in, size_t buf_sz);
    esp_err_t make_room(size_t *free_idx, size_t space_needed);
};

