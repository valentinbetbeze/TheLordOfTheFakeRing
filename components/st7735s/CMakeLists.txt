set(LIB
    driver
    esp_rom
    esp_common
)

idf_component_register(
                        SRCS "st7735s.c"
                        INCLUDE_DIRS "include"
                        REQUIRES ${LIB}
)
