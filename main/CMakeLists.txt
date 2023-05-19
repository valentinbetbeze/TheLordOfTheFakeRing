set (SOURCES
    "TheLordOfTheFakeRing.c"
)

set(LIB
    esp_rom
    driver
    gamepad
    st7735s
    game_engine
    assets
)

idf_component_register(
                        SRCS ${SOURCES}
                        INCLUDE_DIRS "." "include"
                        REQUIRES ${LIB}
)
