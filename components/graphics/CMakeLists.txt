set(LIB
    
)

idf_component_register(
                        SRCS "graphics.c"
                        INCLUDE_DIRS "include"
                        REQUIRES ${LIB}
)
