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
#include <assert.h>

#include "driver/spi_common.h"
#include "st7735s_hal.h"


/*************************************************
 * Font parameters
 *************************************************/
#define FONT_SIZE           (6)         // in pixel
#define FIRST_ASCII         (' ')
#define LAST_ASCII          ('Z')
#define LUMA_THRESHOLD      45


/*************************************************
 * Color codes (RGB565)
 ************************************************/
#define RED                 (SPI_SWAP_DATA_TX(0xF800, 16))
#define GREEN               (SPI_SWAP_DATA_TX(0x07E0, 16))
#define BLUE                (SPI_SWAP_DATA_TX(0x001F, 16))
#define ORANGE              (SPI_SWAP_DATA_TX(0xFC60, 16))
#define DARK_GREEN          (SPI_SWAP_DATA_TX(0x2306, 16))
#define LIGHT_BLUE          (SPI_SWAP_DATA_TX(0xD7DF, 16))
#define YELLOW              (SPI_SWAP_DATA_TX(0xF7E0, 16))
#define YELLOW_1            (SPI_SWAP_DATA_TX(0xF7F1, 16))
#define PURPLE              (SPI_SWAP_DATA_TX(0x4169, 16))
#define WHITE               (0xFFFF)
#define BLACK               (0x0000)
#define GREY                (SPI_SWAP_DATA_TX(0xC658, 16))


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
    float alpha;          
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
    float alpha;
} circle_t;

/**
 * @brief Text object to be displayed onto the frame. The size
 * parameter refer to the length of the text, in bytes.
 */
typedef struct {
    uint8_t pos_x;          // Top-left x-position    
    uint8_t pos_y;          // Top-left y-position
    uint8_t adaptive;       /* Make the text color adaptive to its environment
                               Light on dark background
                               Dark on light background */
    uint16_t background;    // Background color, 0 for no background
    uint16_t color;         // Text color (16-bit format)
    float alpha;      
    uint8_t size;           // Text size in bytes
    const uint8_t (*font)[FONT_SIZE];
    const char *data;       // Ptr to char array
} text_t;

/**
 * @brief Sprite object to be displayed onto the frame.
 */
typedef struct {
    uint8_t flip_x :    1;  // Flip sprite on x-axis
    uint8_t flip_y :    1;  // Flip sprite on y-axis
    uint8_t CW_90 :     1;  // 90° clockwise rotation
    uint8_t ACW_90 :    1;  // 90° anti-clockwise rotation
    uint8_t height;         // Width in pixels
    uint8_t width;          // Width in pixels
    int16_t pos_x;          // Top-left x-position
    int16_t pos_y;          // Top-left y-position
    uint16_t background_color;
    float alpha;
    const uint16_t *data;   // Pointer to sprite data
} sprite_t;


/*************************************************
 * Prototypes
 *************************************************/

/**
 * @brief Fill the background color of the frame.
 * 
 * @param[in] color Background color
 */
void st7735s_fill_background(const uint16_t color);

/**
 * @brief Draw a rectangle on the frame.
 * 
 * @param[in] rectangle Pointer to the rectangle object the draw.
 */
void st7735s_draw_rectangle(const rectangle_t *rectangle);

/**
 * @brief Draw a circle on the frame.
 * 
 * @param[in] circle Pointer to the circle object the draw.
 */
void st7735s_draw_circle(const circle_t *circle);

/**
 * @brief Display a text on the frame.
 * 
 * @param[in] text Pointer to the text object the draw.
 * 
 * @note 1. Padding on the x and y directions can be modified in the
 * project configuration header file (project_config.h).
 * @note 2. Currently, draw_text() only supports the use of one font.
 */
void st7735s_draw_text(const text_t *text);

/**
 * @brief Draw a sprite on the frame.
 * 
 * @param[in] sprite Pointer to the sprite object the draw.
 * 
 * @note  The color white, code 0xFFFF, is considered as 
 * transparent by the function, and hence will not be sent
 * to the frame.
 */
void st7735s_draw_sprite(const sprite_t *sprite);


#endif // __ST7735S_GRAPHICS_H__