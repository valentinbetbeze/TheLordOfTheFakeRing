/**
 * @file game_engine.h
 * @author valentin betbeze (valentin.betbeze@gmail.com)
 * @brief Includes interface functions of the game.
 * @date 2023-04-21
 * 
 * @warning Do not modify any value between parenthesis '()'.
 */

#ifndef __GAME_ENGINE_H__
#define __GAME_ENGINE_H__


#include "project_config.h"
#include "st7735s_graphics.h"
#include "maps.h"
#include "fonts.h"
#include "sprites.h"


/*************************************************
 * Data structures
 *************************************************/

typedef struct {
    uint16_t id :           12;
    uint8_t hit :           1;
    uint8_t destroyed :     1;
    uint8_t has_item :      1;
} block_state_t;

typedef struct {
    uint8_t life :              4;    
    uint8_t top_collision :     1;
    uint8_t bottom_collision :  1;
    uint8_t left_collision :    1;
    uint8_t right_collision :   1;
    uint8_t jumping :           1;
    uint8_t accelerating :      1;
    uint8_t speed_x;
    uint8_t speed_y;
    uint32_t t0;
    sprite_t sprite;
} character_t;


/*************************************************
 * Prototypes
 *************************************************/

/**
 * @brief Get the block state object
 * 
 * @param id 
 * @return block_state_t* 
 */
block_state_t *get_block_state(uint16_t id);

/**
 * @brief Check  collisions between the character and environment blocks.
 * 
 * @param[in] map Game map the character is in.
 * @param[in] character Pointer to the character to check for collisions.
 * @param[in] map_x Position of the current frame (x-axis), in pixels.
 * 
 * @return -1 if error, else the number of collisions.
 * 
 * @warning The position of the collision is taken from the reference of the given
 * character. Hence, a 'left' collision means that the sprite has a collision on its
 * left edge.
 */
int8_t check_collisions(const int8_t map[][NB_BLOCKS_Y], character_t *character, uint16_t map_x);

/**
 * @brief Build the frame to display at the current map location.
 * 
 * @param[in] map Game map the character is in.
 * @param[in] map_x Position of the current frame (x-axis), in pixels.
 * @return 1 if error; 0 if success
 * 
 * @warning This function only draws the static elements of the map (blocks).
 * Any interactive element (characters, text, etc), must be drawn seperately.
 */
uint8_t build_frame(const int8_t map[][NB_BLOCKS_Y], uint16_t map_x);


#endif // __GAME_ENGINE_H__