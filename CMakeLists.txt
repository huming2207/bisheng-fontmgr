idf_component_register(
        SRCS
            "external/includes/stb_truetype.h"
            "external/libschrift/schrift.c" "external/libschrift/schrift.h"
            "includes/stb_font_view.hpp"
            "font_cacher.cpp" "includes/font_cacher.hpp"
        INCLUDE_DIRS
            "includes" "external/includes" "external/libschrift"
)