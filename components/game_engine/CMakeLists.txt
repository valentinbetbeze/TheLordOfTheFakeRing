set (SOURCES
    "game_engine_blocks.c"
    "game_engine_char.c"
    "game_engine_platforms.c"
    "game_engine_display.c"
)

set(LIB
    st7735s
    assets
)

idf_component_register(
                        SRCS ${SOURCES}
                        INCLUDE_DIRS "include"
                        REQUIRES ${LIB}
)
