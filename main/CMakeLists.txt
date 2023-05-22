set (SOURCES
    "TheLordOfTheFakeRing.c"
)

set(LIB
    esp_rom
    driver
    gamepad_driver
    ST7735S_driver
    MH-FMD_driver
    game_engine
    assets
)

idf_component_register(
                        SRCS ${SOURCES}
                        INCLUDE_DIRS "." "include"
                        REQUIRES ${LIB}
)
