idf_component_register(
        SRCS
            "includes/font_view.hpp"
            "font_cacher.cpp" "includes/font_cacher.hpp"
            "font_disk_cacher.cpp" "includes/font_disk_cacher.hpp"
        INCLUDE_DIRS
            "includes" "external/includes"

        REQUIRES
            lvgl esp_timer
)