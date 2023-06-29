set (SOURCES
    "ble_server.c"
    "utils.c"
)

set(LIB
    log
    bt
    gamepad_driver
)

idf_component_register(
                        SRCS ${SOURCES}
                        INCLUDE_DIRS "." "include"
                        REQUIRES ${LIB}
)
