#include "graphics.h"

uint16_t RGB888_to_RGB565(uint8_t red, uint8_t green, uint8_t blue)
{
    red = (uint8_t) ((red * 31) / 255);
    green = (uint8_t) ((green * 63) / 255);
    blue = (uint8_t) ((blue * 31) / 255);
    uint16_t rgb565 = ((red << 11) & RED) | ((green << 5) & GREEN) | (blue & BLUE);
    /* Swap data to send MSB first (required for compatibility with ST7735S).
     See https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/spi_master.html#transactions-with-integers-other-than-uint8-t
    */
    return SPI_SWAP_DATA_TX(rgb565, 16);
}


uint16_t hex_RGB888_to_RGB565(uint32_t rgb888)
{
    // Convert each color in the new format
    uint8_t red = (uint8_t) (((rgb888 >> 16) * 31) / 255);
    uint8_t green = (uint8_t) (((rgb888 >> 8 & 0xFF) * 63) / 255);
    uint8_t blue = (uint8_t) (((rgb888 & 0xFF) * 31) / 255);
    // Compile in one code
    uint16_t rgb565 = ((red << 11) & RED) | ((green << 5) & GREEN) | (blue & BLUE);
    /* Swap data to send MSB first (required for compatibility with ST7735S).
     See https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/spi_master.html#transactions-with-integers-other-than-uint8-t
    */
    return SPI_SWAP_DATA_TX(rgb565, 16);
}
