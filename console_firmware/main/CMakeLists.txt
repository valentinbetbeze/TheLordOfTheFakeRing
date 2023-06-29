set (SOURCES
    "console_firmware.c"
    "utils.c"
)

set(LIB
    esp_system
    driver
    ST7735S_driver
    MH-FMD_driver
    game_engine
    assets
    soc
    nimBLE_client
)

idf_component_register(
                        SRCS ${SOURCES}
                        INCLUDE_DIRS "." "include"
                        REQUIRES ${LIB}
)
