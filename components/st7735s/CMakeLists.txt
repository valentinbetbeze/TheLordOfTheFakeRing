set (SOURCES
    "st7735s_hal.c"
    "st7735s_graphics.c"
)

set(LIB
    driver
    esp_rom
    esp_common
)

idf_component_register(
                        SRCS ${SOURCES}
                        INCLUDE_DIRS "include"
                        REQUIRES ${LIB}
)
