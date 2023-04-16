set(LIB
    driver
    esp_adc
    esp_timer
    esp_common
)

idf_component_register(
                        SRCS "joystick.c"
                        INCLUDE_DIRS "include"
                        REQUIRES ${LIB}
)
