/**
 * @file maps.h
 * @author valentin betbeze (valentin.betbeze@gmail.com)
 * @brief Header file containing the game maps.
 * @date 2023-04-21
 * 
 * @warning Do not modify any value between parenthesis '()'.
 */

#ifndef __MAPS_H__
#define __MAPS_H__


#include <stdint.h>

#include "st7735s_graphics.h"


/**
 * @brief A 'block' is a sprite representing an environmental element
 * of the game (stone, earth, grass, etc). It has fixed width and height
 * and is square.
 */
#define BLOCK_SIZE          (16)       /* Block size in pixel            */
#define NUM_BLOCKS_X        (10)       /* Number of blocks on the x-axis */
#define NUM_BLOCKS_Y        (8)        /* Number of blocks on the y-axis */


/*************************************************
 * Maps
 *************************************************/

#define SHIRE               (1)
#define MORIA               (2)

typedef struct {
    uint8_t id;
    uint16_t background_color;
    uint16_t rows;
    uint8_t columns;
    const int8_t (*data)[NUM_BLOCKS_Y];
} map_t;

extern map_t map_shire;
extern map_t map_moria;

#endif // __MAPS_H__