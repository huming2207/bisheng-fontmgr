idf_component_register(
        SRCS
            "external/includes/stb_truetype.h"
            "includes/font_view.hpp"
            "font_cacher.cpp" "includes/font_cacher.hpp"
            "font_disk_cacher.cpp" "includes/font_disk_cacher.hpp"
            "stbtt_mempool.c" "includes/stbtt_mempool.h"
            "external/tlsf.c" "external/includes/tlsf.h"
        INCLUDE_DIRS
            "includes" "external/includes"

        REQUIRES
            lvgl esp_timer
)