/**
 * @file st7735s_graphics.h
 * @author valentin betbeze (valentin.betbeze@gmail.com)
 * @brief Header file for the graphics functions of the ST7735S driver.
 * Compatible for use with: ESP32-WROOM-32.
 * @date 2023-04-19
 * 
 * @warning Do not modify any value between parenthesis '()'.
 */

#ifndef __ST7735S_GRAPHICS_H__
#define __ST7735S_GRAPHICS_H__


#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include "driver/spi_common.h"
#include "project_config.h"
#include "st7735s_fonts.h"


/*************************************************
 * Some color codes (RGB565)
 ************************************************/
#define RED                 (0xF800)
#define GREEN               (0x07E0)
#define BLUE                (0x001F)
#define WHITE               (0xFFFF)
#define BLACK               (0x0000)


/*************************************************
 * Extern variables
 *************************************************/
extern uint16_t frame[NUM_TRANSACTIONS][PX_PER_TRANSACTION];


/*************************************************
 * Data structures
 *************************************************/
typedef enum {
    BACKGROUND,
    RECTANGLE,
    CIRCLE,
    TEXT,
    SPRITE
} item_type_t;

/**
 * @brief 
 * 
 */
typedef struct {
    uint8_t pos_x;          /* top-left x-position  */
    uint8_t pos_y;          /* top-left y-position  */
    uint8_t height;         /* in pixels            */
    uint8_t width;          /* in pixels            */
    uint16_t color;         /* 16-bit format        */
} rectangle_t;

/**
 * @brief 
 * 
 */
typedef struct {
    uint8_t pos_x;          /* center x-position    */
    uint8_t pos_y;          /* center y-position    */
    uint8_t radius;         /* in pixels            */
    uint8_t thickness;      /* in pixels            */
    uint16_t color;         /* 16-bit format        */
} circle_t;

/**
 * @brief 
 * 
 */
typedef struct {
    uint8_t pos_x;
    uint8_t pos_y;
    uint16_t color;         /* 16-bit format        */
    uint8_t size;           /* in bytes             */
    uint8_t *data;          /* ptr to char array    */
} text_t;

/**
 * @brief 
 * 
 */
typedef struct {
    uint8_t pos_x;          /* top-left x-position  */
    uint8_t pos_y;          /* top-left y-position  */
    uint8_t size;           /* in bytes             */
    uint16_t *data;         /* ptr to sprite data   */
} sprite_t;

/**
 * @brief 
 * 
 */
typedef struct {
    item_type_t type;
    union {
        uint16_t background_color;
        rectangle_t rectangle;
        circle_t circle;
        text_t text;
        sprite_t sprite;
    };
} item_t;


/*************************************************
 * Prototypes
 *************************************************/

/**
 * @brief 
 * 
 * @param x 
 * @param y 
 * @param row 
 * @param column 
 */
void get_frame_indexes(uint8_t x, uint8_t y, uint16_t *row, uint16_t *column);

/**
 * @brief Convert a RGB888 color code (24-bit) to RGB565 (16-bit)
 * 
 * @param rgb888 24-bit color code
 * @return The corresponding 16-bit color code
 */
uint16_t RGB565(uint32_t rgb888);

/**
 * @brief Fill the background color of the frame.
 * 
 * @param color Background color
 */
void fill_background(uint16_t color);

/**
 * @brief 
 * 
 * @param rectangle 
 */
void draw_rectangle(rectangle_t rectangle);

/**
 * @brief 
 * 
 * @param circle 
 */
void draw_circle(circle_t circle);

/**
 * @brief 
 * 
 * @param text 
 */
void draw_text(text_t text);

/**
 * @brief 
 * 
 * @param sprite 
 */
void draw_sprite(sprite_t sprite);

/**
 * @brief 
 * 
 * @param item 
 * @param nitems
 */
void build_frame(item_t *item, int nitems);


#endif // __ST7735S_GRAPHICS_H__