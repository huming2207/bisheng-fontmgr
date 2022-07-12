idf_component_register(
        SRCS
            "external/includes/stb_truetype.h"
            "includes/font_view.hpp"
            "font_cacher.cpp" "includes/font_cacher.hpp"
            "font_disk_cacher.cpp" "includes/font_disk_cacher.hpp"
            "external/font_tlsf.c" "external/includes/font_tlsf.h"
            "font_view.cpp"
        INCLUDE_DIRS
            "includes" "external/includes"

        REQUIRES
            lvgl esp_timer
)