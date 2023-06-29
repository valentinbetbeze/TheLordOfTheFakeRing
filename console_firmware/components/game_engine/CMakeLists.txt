set (SOURCES
    "game_engine_blocks.c"
    "game_engine_char.c"
    "game_engine_platforms.c"
    "game_engine_display.c"
    "game_engine_utils.c"
)

set(LIB
    ST7735S_driver
    MH-FMD_driver
    assets
)

idf_component_register(
                        SRCS ${SOURCES}
                        INCLUDE_DIRS "include"
                        REQUIRES ${LIB}
)
