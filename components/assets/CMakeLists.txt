set (SOURCES
    "fonts.c"
    "maps.c"
    "sprites.c"
)

set(LIB
    st7735s
)

idf_component_register(
                        SRCS ${SOURCES}
                        INCLUDE_DIRS "include"
                        REQUIRES ${LIB}
)
