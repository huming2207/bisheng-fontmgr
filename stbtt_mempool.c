#include <stdbool.h>
#include <esp_heap_caps.h>

#include "stbtt_mempool.h"

#define STBTT_MEM_INIT_SIZE 131072

static void *stbtt_main_heap = NULL;

esp_err_t stbtt_mem_init()
{
    if (stbtt_main_heap) {
        return ESP_OK;
    }


#ifdef CONFIG_SPIRAM
    stbtt_main_heap = heap_caps_aligned_calloc(4, STBTT_MEM_INIT_SIZE, 1, MALLOC_CAP_SPIRAM);
#else
    stbtt_main_heap = heap_caps_aligned_calloc(4, STBTT_MEM_INIT_SIZE, 1, MALLOC_CAP_DEFAULT);
#endif

    if (stbtt_main_heap == NULL) {
        return ESP_ERR_NO_MEM;
    }

    stbtt_mem_tlsf = stbtt_tlsf_create_with_pool(stbtt_main_heap, STBTT_MEM_INIT_SIZE);
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
