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
    uint8_t destroyed :         1;
    uint8_t has_item :          1;
    uint8_t bumping :           1;
    int16_t row;
    int8_t column;
} block_t;

typedef struct {
    uint8_t top_collision :     1;
    uint8_t bottom_collision :  1;
    uint8_t left_collision :    1;
    uint8_t right_collision :   1;
    uint8_t falling :           1;
    uint8_t jumping :           1;
    uint8_t accelerating :      1;
    uint8_t free_flag :         1;
    int16_t pos_x;
    int16_t pos_y;
    uint8_t speed_x;
    uint8_t speed_y;
} physics_t;

typedef struct {
    uint8_t life :              1;
    uint8_t spawned :           1;
    int16_t row;
    int8_t column;
    uint32_t timer;
    physics_t physics;
} enemy_t;

typedef struct {
    uint8_t life :              4;
    uint8_t shield :            1;
    uint8_t firestaff :         1;
    uint8_t mount :             1;
    uint8_t free_flag :         1;
    uint32_t timer;
    physics_t physics;
    sprite_t sprite;
} character_t;


/*************************************************
 * Prototypes
 *************************************************/


uint8_t enter_block_record(block_t block, uint16_t map_row);


block_t *get_block_record(int16_t row, int8_t column);

/**
 * @brief Check  collisions between the character and environment blocks.
 * 
 * @param[in] map Game map the character is in.
 * @param[in] physics Pointer to the physical data of the reference element.
 * @param[in] map_x Position of the current frame (x-axis), in pixels.
 * 
 * @return 0 on success, 1 on failure
 * 
 * @warning The position of the collision is taken from the reference of the given
 * physical object. Hence, a 'left' collision means that the object has a collision on its
 * left side.
 */
uint8_t check_block_collisions(const int8_t map[][NB_BLOCKS_Y], physics_t *physics, uint16_t map_x);

void bump(block_t *block, sprite_t *sprite, uint64_t timer);

#endif // __GAME_ENGINE_H__