set(LIB
    driver
)

idf_component_register(
                        INCLUDE_DIRS "include"
                        REQUIRES ${LIB}
)