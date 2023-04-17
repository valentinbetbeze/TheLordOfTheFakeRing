set(LIB
    joystick
    st7735s
    graphics
    esp_rom
)

idf_component_register(
                        SRCS "TheLordOfTheFakeRing.c"
                        INCLUDE_DIRS "."
                        REQUIRES ${LIB}
)
