#include <stdbool.h>
#include <esp_heap_caps.h>
#include <esp_log.h>

#include "stbtt_mempool.h"

#define STBTT_MEM_INIT_SIZE 131072

#ifdef TAG
#undef TAG
#endif

#define TAG "stbtt_heap"

static void *stbtt_main_heap = NULL;

esp_err_t stbtt_mem_init()
{
    if (stbtt_main_heap) {
        ESP_LOGI(TAG, "Heap already initialised");
        return ESP_OK;
    }


#ifdef CONFIG_SPIRAM
    stbtt_main_heap = heap_caps_aligned_calloc(4, STBTT_MEM_INIT_SIZE, 1, MALLOC_CAP_SPIRAM);
#else
    stbtt_main_heap = heap_caps_aligned_calloc(4, STBTT_MEM_INIT_SIZE, 1, MALLOC_CAP_DEFAULT);
#endif

    if (stbtt_main_heap == NULL) {
        ESP_LOGI(TAG, "Failed to alloc heap");
        return ESP_ERR_NO_MEM;
    }

    stbtt_mem_tlsf = stbtt_tlsf_create_with_pool(stbtt_main_heap, STBTT_MEM_INIT_SIZE);
    if (stbtt_mem_tlsf == NULL) {
        ESP_LOGE(TAG, "Failed to create heap mem pool");
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}

void *stbtt_mem_alloc(size_t len)
{
    return stbtt_tlsf_malloc(stbtt_mem_tlsf, len);
}

void stbtt_mem_free(void *ptr)
{
    free(ptr);
}
