#pragma once

#include <esp_err.h>
#include <stdbool.h>
#include "tlsf.h"

#ifdef __cplusplus
extern "C" {
#endif

static stbtt_tlsf_t stbtt_mem_tlsf = NULL;

esp_err_t stbtt_mem_init();
void *stbtt_mem_alloc(size_t len);
void stbtt_mem_free(void *ptr);

#define STBTT_malloc(x,u)  ((void)(u),stbtt_mem_alloc(x))
#define STBTT_free(x,u)    ((void)(u),stbtt_mem_free(x))

#ifdef __cplusplus
}
#endif