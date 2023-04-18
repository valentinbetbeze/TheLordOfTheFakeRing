#include <stdio.h>
#include <stdint.h>

#include "driver/spi_common.h"

/*************************************************
 * Some color codes (RGB565)
 ************************************************/
#define RED                 (0xF800)
#define GREEN               (0x07E0)
#define BLUE                (0x001F)
#define WHITE               (0xFFFF)
#define BLACK               (0x0000)


/**
 * @brief Convert a RGB888 color code (24-bit) to RGB565 (16-bit)
 * 
 * @param rgb888 24-bit color code
 * @return The corresponding 16-bit color code
 */
uint16_t RGB565(uint32_t rgb888);
