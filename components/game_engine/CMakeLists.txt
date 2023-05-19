set (SOURCES
    "game_engine_blocks.c"
    "game_engine_char.c"
    "game_engine_platforms.c"
)

set(LIB
    st7735s
)

idf_component_register(
                        SRCS ${SOURCES}
                        INCLUDE_DIRS "include"
                        REQUIRES ${LIB}
)
