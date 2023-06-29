set (SOURCES
    "ble_client.c"
    "misc.c"
    "peer.c"
)

set(LIB
    log
    nvs_flash
    bt
    freertos
    soc
)

idf_component_register(
                        SRCS ${SOURCES}
                        INCLUDE_DIRS "." "include"
                        REQUIRES ${LIB}
)
