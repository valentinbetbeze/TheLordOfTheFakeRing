set (SOURCES
    "fonts.c"
    "maps.c"
    "sprites.c"
)

idf_component_register(
                        SRCS ${SOURCES}
                        INCLUDE_DIRS "include"
)
