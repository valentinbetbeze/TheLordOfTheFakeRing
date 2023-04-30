set (SOURCES
    "TheLordOfTheFakeRing.c"
    "game_engine.c"
)

set(LIB
    project_config
    gamepad
    st7735s
    esp_rom
    driver
)

idf_component_register(
                        SRCS ${SOURCES}
                        INCLUDE_DIRS "." "include"
                        REQUIRES ${LIB}
)
