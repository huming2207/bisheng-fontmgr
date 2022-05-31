#pragma once

#include <cstdio>
#include <esp_err.h>

typedef size_t (*get_part_free_space_fn)(void *);

class font_disk_cacher
{
public:
    static font_disk_cacher& instance()
    {
        static font_disk_cacher cacher;
        return cacher;
    }

    void operator=(font_disk_cacher const&) = delete;
    font_disk_cacher(font_disk_cacher const&) = delete;

public:
    esp_err_t init(const char *_base_path, get_part_free_space_fn _free_space_getter_fn, void *_free_space_getter_ctx);
    esp_err_t add_renderer(const char *font_name, uint8_t font_size);
    esp_err_t add_bitmap(const char *font_name, uint8_t font_size, uint32_t codepoint, uint8_t *buf, size_t len);
    esp_err_t get_bitmap(const char *font_name, uint8_t font_size, uint32_t codepoint, uint8_t *buf_out, size_t len, size_t *len_out);
    esp_err_t delete_all();

private:
    font_disk_cacher() = default;

private:
    get_part_free_space_fn free_space_getter_fn = nullptr;
    void *get_free_space_fn_ctx = nullptr;
    char *base_path = nullptr;
    static const constexpr char *TAG = "ft_disk_cache";
};
