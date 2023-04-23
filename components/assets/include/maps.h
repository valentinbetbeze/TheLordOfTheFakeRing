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

/**
 * @brief A 'block' is a sprite representing an environmental element
 * of the game (stone, earth, grass, etc). It has fixed width and height
 * and is square.
 */
#define BLOCK_SIZE          (16)       /* Block size in pixel            */
#define NB_BLOCKS_X         (10)       /* Number of blocks on the x-axis */
#define NB_BLOCKS_Y         (8)        /* Number of blocks on the y-axis */


/*************************************************
 * Maps
 *************************************************/
extern const uint8_t shire[][BLOCK_SIZE];


#endif // __MAPS_H__