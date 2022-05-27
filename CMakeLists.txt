idf_component_register(
        SRCS
            "external/includes/stb_truetype.h"
            "font_view.cpp" "includes/font_view.hpp"
            "font_cacher.cpp" "includes/font_cacher.hpp"
        INCLUDE_DIRS
            "includes" "external/includes"

        REQUIRES
            lvgl esp_timer
)