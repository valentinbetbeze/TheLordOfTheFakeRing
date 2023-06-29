set (SOURCES
    "fonts.c"
    "maps.c"
    "sprites.c"
    "musics.c"
)

set(LIB
    ST7735S_driver
    game_engine
)

idf_component_register(
                        SRCS ${SOURCES}
                        INCLUDE_DIRS "include"
                        REQUIRES ${LIB}
)
