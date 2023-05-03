/**
 * @file sprites.h
 * @author valentin betbeze (valentin.betbeze@gmail.com)
 * @brief Header file containing the sprites data.
 * @date 2023-04-22
 * 
 * @warning Do not modify any value between parenthesis '()'.
 */

#ifndef __SPRITES_H__
#define __SPRITES_H__


#include <stdint.h>
#include "maps.h"


/*************************************************
 * Sprites
 *************************************************/
extern const uint16_t sprite_player[BLOCK_SIZE*BLOCK_SIZE];

extern const uint16_t shire_block_1[BLOCK_SIZE*BLOCK_SIZE];
extern const uint16_t shire_block_2[BLOCK_SIZE*BLOCK_SIZE];
extern const uint16_t shire_block_3[BLOCK_SIZE*BLOCK_SIZE];
extern const uint16_t shire_enemy_1[BLOCK_SIZE*BLOCK_SIZE];

extern const uint16_t moria_block_1[BLOCK_SIZE*BLOCK_SIZE];
extern const uint16_t moria_block_2[BLOCK_SIZE*BLOCK_SIZE];
extern const uint16_t moria_block_3[BLOCK_SIZE*BLOCK_SIZE];



#endif // __SPRITES_H__