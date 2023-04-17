#include <stdio.h>
#include <stdint.h>

/*************************************************
 * Some color codes (RGB565)
 ************************************************/
#define RED                 (0xF800)
#define GREEN               (0x07E0)
#define BLUE                (0x001F)
#define WHITE               (0xFFFF)
#define BLACK               (0x0000)


/**
 * @brief 
 * 
 * @param red 
 * @param green 
 * @param blue 
 * @return uint16_t 
 */
uint16_t RGB888_to_RGB565(uint8_t red, uint8_t green, uint8_t blue);

/**
 * @brief 
 * 
 * @param rgb888 
 * @return uint16_t 
 */
uint16_t hex_RGB888_to_RGB565(uint32_t rgb888);
