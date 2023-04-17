#include "graphics.h"

uint16_t RGB888_to_RGB565(uint8_t red, uint8_t green, uint8_t blue)
{
    red = (uint8_t) ((red * 31) / 255);
    green = (uint8_t) ((green * 63) / 255);
    blue = (uint8_t) ((blue * 31) / 255);
    uint16_t rgb565 = ((red << 11) & RED) | ((green << 5) & GREEN) | (blue & BLUE);
    return rgb565;
}


uint16_t hex_RGB888_to_RGB565(uint32_t rgb888)
{
    uint8_t red = (uint8_t) (((rgb888 >> 16) * 31) / 255);
    uint8_t green = (uint8_t) (((rgb888 >> 8 & 0xFF) * 63) / 255);
    uint8_t blue = (uint8_t) (((rgb888 & 0xFF) * 31) / 255);
    uint16_t rgb565 = ((red << 11) & RED) | ((green << 5) & GREEN) | (blue & BLUE);
    return rgb565;
}
