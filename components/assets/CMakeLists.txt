set (SOURCES
    "fonts.c"
)

set(LIB
    
)

idf_component_register(
                        SRCS ${SOURCES}
                        INCLUDE_DIRS "include"
                        REQUIRES ${LIB}
)
