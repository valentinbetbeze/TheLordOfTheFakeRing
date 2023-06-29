set (SOURCES
    "controller_firmware.c"
)

set(LIB
    nvs_flash
    gamepad_driver
    nimBLE_server
)

idf_component_register(
                        SRCS ${SOURCES}
                        INCLUDE_DIRS "."
                        REQUIRES ${LIB}
)
