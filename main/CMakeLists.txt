set (SOURCES
    "TheLordOfTheFakeRing.c"
    "interface.c"
)

set(LIB
    project_config
    gamepad
    st7735s
    esp_rom
)

idf_component_register(
                        SRCS ${SOURCES}
                        INCLUDE_DIRS "." "include"
                        REQUIRES ${LIB}
)
