/**
 * @file st7735s_graphics.h
 * @author valentin betbeze (valentin.betbeze@gmail.com)
 * @brief Header file for the graphics functions of the ST7735S driver.
 * Compatible for use with: ESP32-WROOM-32.
 * @date 2023-04-19
 * 
 * @note The `frame` refers to a 2D-array with specific column and row
 * sizes for efficient SPI transfer. See st7735s_hal.h
 * @warning Do not modify any value between parenthesis '()'.
 */

#ifndef __ST7735S_GRAPHICS_H__
#define __ST7735S_GRAPHICS_H__


#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include "driver/spi_common.h"
#include "project_config.h"
#include "fonts.h"


/*************************************************
 * Color codes (RGB565)
 ************************************************/
#define RED                 (SPI_SWAP_DATA_TX(0xF800, 16))
#define GREEN               (SPI_SWAP_DATA_TX(0x07E0, 16))
#define BLUE                (SPI_SWAP_DATA_TX(0x001F, 16))
#define WHITE               (SPI_SWAP_DATA_TX(0xFFDF, 16))
#define BLACK               (0x0000)
#define TRANSPARENT         (0xFFFF) // Full white = transparency


/*************************************************
 * External variables
 *************************************************/
extern uint16_t frame[NUM_TRANSACTIONS][PX_PER_TRANSACTION];


/*************************************************
 * Data structures
 *************************************************/

/**
 * @brief Rectangle object to be displayed onto the frame.
 * @note A line can be made with a thickness of 1 and the desired
 * height/width.
 */
typedef struct {
    uint8_t pos_x;          // Top-left x-position    
    uint8_t pos_y;          // Top-left y-position    
    uint8_t height;         // Width in pixels        
    uint8_t width;          // Width in pixels        
    uint16_t color;         // 16-bit format          
} rectangle_t;

/**
 * @brief Circle object to be displayed onto the frame.
 * @note For a fully filled circle, use a thickness of 0. Else,
 * the circle thickness is drawn inwards.
 */
typedef struct {
    uint8_t pos_x;          // Center x-position      
    uint8_t pos_y;          // Center y-position      
    uint8_t radius;         // Radius in pixels       
    uint8_t thickness;      // Thinkness in pixels    
    uint16_t color;         // 16-bit format          
} circle_t;

/**
 * @brief Text object to be displayed onto the frame. The size
 * parameter refer to the length of the text, in bytes.
 */
typedef struct {
    uint8_t pos_x;          // Top-left x-position    
    uint8_t pos_y;          // Top-left y-position    
    uint16_t color;         // 16-bit format          
    uint8_t size;           // Text size in bytes     
    uint8_t *data;          // Ptr to char array      
} text_t;

/**
 * @brief Sprite object to be displayed onto the frame.
 */
typedef struct {
    int16_t pos_x;          // Top-left x-position    
    int16_t pos_y;          // Top-left y-position    
    uint8_t height;         // Width in pixels        
    uint8_t width;          // Width in pixels        
    const uint16_t *data;   // Pointer to sprite data 
} sprite_t;


/*************************************************
 * Prototypes
 *************************************************/

/**
 * @brief Convert a RGB888 color code (24-bit) to RGB565 (16-bit)
 * 
 * @param rgb888 24-bit color code
 * @return The corresponding 16-bit color code
 */
uint16_t st7735s_rgb565(uint32_t rgb888);

/**
 * @brief Fill the background color of the frame.
 * 
 * @param color Background color
 */
void st7735s_fill_background(uint16_t color);

/**
 * @brief Draw a rectangle on the frame.
 * 
 * @param rectangle Rectangle object.
 */
void st7735s_draw_rectangle(rectangle_t rectangle);

/**
 * @brief Draw a circle on the frame.
 * 
 * @param circle Circle object.
 */
void st7735s_draw_circle(circle_t circle);

/**
 * @brief Display a text on the frame.
 * 
 * @param text Text object.
 * @note 1. Padding on the x and y directions can be modified in the
 * project configuration header file (project_config.h).
 * @note 2. Currently, draw_text() only supports the use of one font.
 */
void st7735s_draw_text(text_t text);

/**
 * @brief Draw a sprite on the frame.
 * 
 * @param sprite Sprite object.
 * @note  The color white, code 0xFFFF, is considered as 
 * transparent by the function, and hence will not be sent
 * to the frame.
 */
void st7735s_draw_sprite(sprite_t sprite);


#endif // __ST7735S_GRAPHICS_H__