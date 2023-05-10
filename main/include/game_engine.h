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

#include <string.h>
#include <math.h>

#include "project_config.h"
#include "st7735s_graphics.h"
#include "maps.h"
#include "fonts.h"
#include "sprites.h"
#include "esp_random.h"


/*************************************************
 * Data structures
 *************************************************/

typedef struct {
    uint8_t top_collision :     1;
    uint8_t bottom_collision :  1;
    uint8_t left_collision :    1;
    uint8_t right_collision :   1;
    uint8_t grounded :          1;      // Cannot fall or jump
    uint8_t falling :           1;
    uint8_t jumping :           1;
    uint8_t accelerating :      1;
    int16_t pos_x;
    int16_t pos_y;
    int8_t speed_x;
    int8_t speed_y;
} physics_t;

typedef struct {
    uint8_t is_hit :            1;
    uint8_t destroyed :         1;
    uint8_t item_given :        1;
    uint8_t bumping :           1;
    int16_t row;
    int8_t column;
} block_t;

typedef enum {
    CUSTOM_SPRITE_1 =           (-1),
    CUSTOM_SPRITE_2 =           (-2),
    CUSTOM_SPRITE_3 =           (-3),
    BACKGROUND_BLOCK =          (0),
    NON_BREAKABLE_BLOCK_1 =     (1),
    NON_BREAKABLE_BLOCK_2 =     (2),
    BREAKABLE_BLOCK =           (3),
    BONUS_BLOCK =               (4)
} block_type_t;

typedef struct {
    int8_t life;
    int16_t row;
    int8_t column;
    uint32_t timer_x;
    uint32_t timer_y;
    physics_t physics;
} enemy_t;

typedef enum {
    ENEMY_1 =                   (-30),
    ENEMY_2 =                   (-31),
    ENEMY_3 =                   (-32),
} enemy_type_t;

typedef struct {
    uint8_t spawned :           1;
    uint8_t taken :             1;
    uint8_t type;
    uint8_t steps;
    uint32_t timer;
    sprite_t sprite;
} item_t;

typedef enum {
    COIN =                      (1),
    LIGHTSTAFF =                (2),
    SHIELD =                    (3),
} item_type_t;

typedef struct {
    uint8_t life :              4;
    uint8_t shield :            1;
    uint8_t lightstaff :        1;
    uint8_t forward :           1;
    uint8_t power_used :        1;
    uint16_t spell_radius;
    uint32_t timer;
    physics_t physics;
    sprite_t sprite;
} character_t;


/*************************************************
 * Macros
 *************************************************/

#define IS_SOLID(x)             (x > BACKGROUND_BLOCK)
#define IS_INTERACTIVE(x)       (x >= BREAKABLE_BLOCK)
#define IS_ENEMY(x)             (x <= ENEMY_1)
#define MAP_BACKGROUND(x)       (x[0][2] << 8 | x[0][1])
#define MAP_ID(x)               (x[0][0])


/*************************************************
 * Prototypes
 *************************************************/

extern block_t blocks[NUM_BLOCK_RECORDS];
extern enemy_t enemies[NUM_ENEMY_RECORDS];
extern item_t items[NUM_ITEMS];

void initialize_blocks_records(void);

uint8_t create_block_record(block_t block, uint16_t map_row);

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
uint8_t check_block_collisions(const int8_t map[][NUM_BLOCKS_Y], physics_t *physics, uint16_t map_x);

void fix_position(physics_t *physics, uint16_t map_x);

void bump_block(block_t *block, sprite_t *sprite, uint64_t timer);

void initialize_enemy(enemy_t *enemy, int16_t row, int8_t column, uint16_t map_row);

uint8_t create_enemy_record(enemy_t enemy);

enemy_t *get_enemy_record(int16_t row, int8_t column);

void spawn_enemies(const int8_t map[][NUM_BLOCKS_Y], int16_t start_row, int16_t end_row, uint16_t map_row);

item_t generate_item(int16_t row, int8_t column, uint16_t map_x);

uint8_t store_item(item_t item);


#endif // __GAME_ENGINE_H__