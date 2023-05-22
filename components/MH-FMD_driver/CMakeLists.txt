set (SOURCES
    "MH-FMD_driver.c"
)

set(LIB
    driver
)

idf_component_register(
                        SRCS ${SOURCES}
                        INCLUDE_DIRS "include"
                        REQUIRES ${LIB}
)