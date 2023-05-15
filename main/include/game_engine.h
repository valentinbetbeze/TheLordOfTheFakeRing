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
    uint8_t grounded :          1;      // Cannot fall in holes (enemies)
    uint8_t falling :           1;
    uint8_t jumping :           1;
    uint8_t accelerating :      1;
    int16_t pos_x;
    int16_t pos_y;
    int8_t speed_x;
    int8_t speed_y;
    int8_t platform_i;                  // Index of the platfom the entity is standing on
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
    PLATFORM_BLOCK =            (-8),
    CUSTOM_SPRITE_3 =           (-3),
    CUSTOM_SPRITE_2 =           (-2),
    CUSTOM_SPRITE_1 =           (-1),    
    BACKGROUND_BLOCK =          (0),
    NON_BREAKABLE_BLOCK_1 =     (1),
    NON_BREAKABLE_BLOCK_2 =     (2),
    BREAKABLE_BLOCK =           (3),
    BONUS_BLOCK =               (4)
} block_type_t;

typedef struct {
    uint8_t horizontal :        1;
    uint8_t vertical :          1;
    uint8_t moved :             1;
    uint8_t changed_dir :       1;
    int16_t start_row;
    int16_t end_row;
    int8_t start_column;
    int8_t end_column;
    uint32_t timer;
    physics_t physics;
} platform_t;

typedef struct {
    int8_t life;
    uint8_t stationary;
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
    BOSS =                      (-33)
} enemy_type_t;

typedef struct {
    float slope;
    int32_t offset;
    uint32_t timer;
    physics_t physics;
} projectile_t;

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
    uint8_t coins;
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

#define PLATFORM_SIZE           (2 * BLOCK_SIZE) // In pixels


/*************************************************
 * Declarations
 *************************************************/

extern block_t blocks[NUM_BLOCK_RECORDS];
extern enemy_t enemies[NUM_ENEMY_RECORDS];
extern item_t items[NUM_ITEMS];
extern platform_t platforms[MAX_PLATFORMS];
extern projectile_t projectiles[MAX_PROJECTILES];

/*************************************************
 * 'Blocks' function prototypes
 *************************************************/

void initialize_blocks_records(void);

uint8_t create_block_record(block_t block, uint16_t map_row);

block_t *get_block_record(int16_t row, int8_t column);

/**
 * @brief Check  collisions between the character and environment blocks.
 * 
 * @param[in] map Game map the character is in.
 * @param[in] physics Pointer to the physical data of the reference element.
 * @param[in] map_row 
 * 
 * @return 1 if collision, 0 if no collision, -1 if critical error
 * 
 * @warning The position of the collision is taken from the reference of the given
 * physical object. Hence, a 'left' collision means that the object has a collision on its
 * left side.
 */
int8_t check_block_collisions(map_t map, physics_t *physics, uint16_t map_row);

void fix_block_collision(physics_t *physics);

void bump_block(block_t *block, sprite_t *sprite, uint64_t timer);



/*************************************************
 * 'Platforms' functions prototypes
 *************************************************/

platform_t *get_platform(int16_t row, int8_t column);

uint8_t load_platforms(map_t map);

int8_t check_platform_collision(physics_t *physics);



/*************************************************
 * 'Enemies / Player' function prototypes
 *************************************************/

void set_stationary_enemy_orientation(map_t map, enemy_t *enemy);

uint8_t create_enemy_record(enemy_t enemy, uint16_t map_x);

enemy_t *get_enemy_record(int16_t row, int8_t column);

uint8_t spawn_enemies(map_t map, uint16_t map_x, int16_t start_row, int16_t end_row);

uint8_t initiate_jump(physics_t *physics, uint8_t initial_speed);

int8_t is_on_sight(map_t map, physics_t *shooter, physics_t *target);

uint8_t initiate_shoot(physics_t *shooter, physics_t *target);


/*************************************************
 * 'Items' function prototypes
 *************************************************/

item_t generate_item(int16_t row, int8_t column, uint16_t map_x);

uint8_t store_item(item_t item);


#endif // __GAME_ENGINE_H__