#include <esp_log.h>
#include <cstring>
#include <dirent.h>
#include <cerrno>
#include <sys/stat.h>
#include "font_disk_cacher.hpp"

#define FT_DISK_CACHE_PATH_OVERHEAD 10
#define FT_DISK_CACHE_DIR_PERMISSION 0777  // This is just a placeholder

esp_err_t font_disk_cacher::init(const char *_base_path, get_part_free_space_fn _free_space_getter_fn, void *_free_space_getter_ctx)
{
    if (_base_path == nullptr || strlen(_base_path) < 1) {
        ESP_LOGE(TAG, "Base path is empty");
        return ESP_ERR_INVALID_ARG;
    } else {
        DIR *dir = opendir(_base_path);
        if (dir != nullptr) {
            closedir(dir); // All good
        } else if (ENOENT == errno) {
            ESP_LOGE(TAG, "Dir open failed, no such base path");
            return ESP_ERR_INVALID_ARG;
        } else {
            ESP_LOGE(TAG, "Dir open failed, errno 0x%x", errno);
            return ESP_ERR_INVALID_STATE;
        }
    }

    base_path = strdup(_base_path);
    free_space_getter_fn = _free_space_getter_fn;
    get_free_space_fn_ctx = _free_space_getter_ctx;

    return ESP_OK;
}

esp_err_t font_disk_cacher::add_renderer(const char *font_name, uint8_t font_size)
{
    char combined_path[256] = { 0 };

    if (strlen(font_name) + strlen(base_path) + FT_DISK_CACHE_PATH_OVERHEAD > sizeof(combined_path)) {
        ESP_LOGE(TAG, "Font path too long");
        return ESP_ERR_NO_MEM;
    }

    snprintf(combined_path, sizeof(combined_path), "%s/%s", base_path, font_name);
    combined_path[sizeof(combined_path) - 1] = '\0';

    // Check if this font family already exists
    DIR *dir = opendir(combined_path);
    if (dir == nullptr) {
        if (mkdir(combined_path, FT_DISK_CACHE_DIR_PERMISSION) != 0) {
            closedir(dir);
            ESP_LOGE(TAG, "Create dir for font family failed");
            return ESP_ERR_INVALID_STATE;
        }
    } else {
        closedir(dir);
        dir = nullptr;
    }

    memset(combined_path, 0, sizeof(combined_path));
    snprintf(combined_path, sizeof(combined_path), "%s/%s/%x", base_path, font_name, font_size);
    combined_path[sizeof(combined_path) - 1] = '\0';

    // Check if this font size dir already exists
    dir = opendir(combined_path);
    if (dir == nullptr) {
        if (mkdir(combined_path, FT_DISK_CACHE_DIR_PERMISSION) != 0) {
            closedir(dir);
            ESP_LOGE(TAG, "Create dir for font size failed");
            return ESP_ERR_INVALID_STATE;
        }
    } else {
        closedir(dir);
        dir = nullptr;
    }

    return ESP_OK;
}

esp_err_t font_disk_cacher::add_bitmap(const char *font_name, uint8_t font_size, uint32_t codepoint, uint8_t *buf, size_t len)
{
    if (free_space_getter_fn != nullptr) {
        size_t free_space = (*free_space_getter_fn)(get_free_space_fn_ctx);
        if (len > free_space) {
            ESP_LOGE(TAG, "Cache full!");
            return ESP_ERR_NO_MEM;
        }
    }

    char combined_path[256] = { 0 };

    snprintf(combined_path, sizeof(combined_path), "%s/%s/%x/%x", base_path, font_name, font_size, codepoint);
    combined_path[sizeof(combined_path) - 1] = '\0';

    FILE *fp = fopen(combined_path, "wb");
    if (fp == nullptr) {
        ESP_LOGE(TAG, "Failed to open path: %s", combined_path);
        return ESP_ERR_INVALID_STATE;
    }

    if (fwrite(buf, 1, len, fp) < len) {
        ESP_LOGE(TAG, "Failed to write to buffer");
        fclose(fp);
        return ESP_ERR_INVALID_STATE;
    }

    fflush(fp);
    fclose(fp);

    return ESP_OK;
}

esp_err_t font_disk_cacher::get_bitmap(const char *font_name, uint8_t font_size, uint32_t codepoint, uint8_t *buf_out, size_t len, size_t *len_out)
{
    char combined_path[256] = { 0 };

    snprintf(combined_path, sizeof(combined_path), "%s/%s/%x/%x", base_path, font_name, font_size, codepoint);
    combined_path[sizeof(combined_path) - 1] = '\0';

    FILE *fp = fopen(combined_path, "rb");
    if (fp == nullptr) {
        if (errno == ENOENT) {
            ESP_LOGE(TAG, "Not found: %s", combined_path);
            return ESP_ERR_NOT_FOUND;
        } else {
            ESP_LOGE(TAG, "Failed to open path: %s", combined_path);
            return ESP_ERR_INVALID_STATE;
        }
    }

    if (len_out != nullptr) {
        fseek(fp, 0, SEEK_END);
        *len_out = ftell(fp);
        rewind(fp);
    }

    if (fread(buf_out, 1, len, fp) < 1) {
        ESP_LOGE(TAG, "Read op failed");
        fclose(fp);
        return ESP_FAIL;
    }

    fclose(fp);
    return ESP_OK;
}

esp_err_t font_disk_cacher::delete_all()
{
    return 0;
}

