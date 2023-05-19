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

#include "st7735s_graphics.h"


/*************************************************
 * Preprocessor directives
 *************************************************/

// Blocks
#define BLOCK_SIZE              (16)        // Block size in pixel
#define NUM_BLOCKS_X            (10)        // Number of blocks on the x-axis 
#define NUM_BLOCKS_Y            (8)         // Number of blocks on the y-axis
#define NUM_BLOCK_RECORDS       10
#define SLIP_OFFSET             2           // Left/right slip offset, in pixels
#define IS_SOLID(x)             (x > BACKGROUND_BLOCK)
#define IS_INTERACTIVE(x)       (x >= BREAKABLE_BLOCK)
// Items
#define NUM_ITEMS               10          // Maximum number of items on one frame.
#define TIMESTEP_BUMP_COIN      5           // In milliseconds
#define HEIGHT_BUMP_COIN        36          // Bump height of a coin, in pixels
#define SHIELD_ALPHA            0.7         // Shield color transparency  
// Player
#define TIMESTEP_ACCEL          200         // y-displacement delay in milliseconds 
#define SPEED_INITIAL           1           // Player base speed
#define SPEED_JUMP_INIT         2           // Jump 'impulsion' speed (v0)
// Enemy
#define SPAWN_START(x)          (x + NUM_BLOCKS_X + 1)  // Starting row for spawning enemies
#define SPAWN_END(x)            (x + NUM_BLOCKS_X + 2)  // Final row for spawning enemies (included)
#define NUM_ENEMY_RECORDS       15          // Maximum number of enemies on 
#define TIMESTEP_ENEMY          15          // x-displacement delay in milliseconds
#define KILL_ZONE_Y             5           // Height, in pixels, in which an enemy is killed
#define IS_ENEMY(x)             (x <= ENEMY_1)
// Projectiles
#define MAX_PROJECTILES         10
#define COOLDOWN_SHOOT          2000        // Cooldown for an enemy to shoot a projectile, in milliseconds
#define HITBOX_PROJECTILE       8           // Square hitbox, in pixels
// Platforms
#define MAX_PLATFORMS           10                  // Maximum number of platforms allowed on a frame
#define SPEED_PLATFORM          1
#define TIMESTEP_PLATFORM       15                  // in milliseconds


/*************************************************
 * Data structures
 *************************************************/

/**
 * @brief The map_t object gathers all necessary information to manipulate 
 * and work with a map in the game engine functions.
 */
typedef struct {
    uint8_t id;
    uint16_t background_color;
    uint16_t nrows;                     // Number of rows
    uint8_t ncolumns;                   // Number of columns
    uint16_t end_row;                   // Row at which the player completes the map
    const int8_t (*data)[NUM_BLOCKS_Y];
} map_t;

/**
 * @brief The physics_t object is used to work with dynamic elements which
 * must interact with their environment. An example of such use is the player's
 * character, which is subject to gravity or collisions.
 * 
 * @param grounded Cannot fall in holes. Mostly useful for enemies.
 * @param platform_i Must be set to -1 when the physics_t is initialized, indicating
 * that the physics_t object is not standing on any platform.
 */
typedef struct {
    uint8_t top_collision :     1;
    uint8_t bottom_collision :  1;
    uint8_t left_collision :    1;
    uint8_t right_collision :   1;
    uint8_t grounded :          1;      // Cannot fall in holes
    uint8_t falling :           1;
    uint8_t jumping :           1;
    uint8_t accelerating :      1;
    int16_t pos_x;
    int16_t pos_y;
    int8_t speed_x;
    int8_t speed_y;
    int8_t platform_i;                  // Index of the platfom the entity is standing on
} physics_t;

/**
 * @brief A block_t object is created whenever an action by the player changes
 * the state of an interactive block. The block is identified thanks to its
 * unique row/column combinaison.
 */
typedef struct {
    uint8_t is_hit :            1;
    uint8_t destroyed :         1;
    uint8_t item_given :        1;
    uint8_t bumping :           1;
    int16_t row;
    int8_t column;
} block_t;

/**
 * @brief Block types. Types whose values are positive are considered solid.
 * Otherwise, they are considered non-solid and physics_t object will not be
 * affected by them (no collision).
 */
typedef enum {
    PLATFORM_BLOCK =            (-8),
    CUSTOM_SPRITE_4 =           (-4),
    CUSTOM_SPRITE_3 =           (-3),
    CUSTOM_SPRITE_2 =           (-2),
    CUSTOM_SPRITE_1 =           (-1),    
    BACKGROUND_BLOCK =          (0),
    NON_BREAKABLE_BLOCK_1 =     (1),
    NON_BREAKABLE_BLOCK_2 =     (2),
    BREAKABLE_BLOCK =           (3),
    BONUS_BLOCK =               (4),
    RING =                      (5)
} block_type_t;

/**
 * @brief An item_t object gather all informations related to the generated
 * item. This includes the members @p steps and @p timer, useful giving
 * the item a danymic animation. 
 */
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

/**
 * @brief Struct for the character of the player. 
 */
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

/**
 * @brief Struct handling the properties of an enemy object.
 * 
 * @param stationary The enemy does not move
 * @param inifite_spawn The enemy spawns again if it falls down a hole.
 * The effect is stopped if the enemy is killed by the player.
 */
typedef struct {
    uint8_t stationary :        1;      // The enemy does not move
    uint8_t infinite_spawn :    1;      // Spawn until killed by the player
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
    ENEMY_4 =                   (-33),
    BOSS =                      (-35)
} enemy_type_t;

/**
 * @brief Struct handling the properties of a projectile.
 * 
 * @param slope Coefficient of the linear equation characterizing the behaviour
 * of the projectile's trajectory. Determined at the creation of the projectile_t object.
 * @param offset Offset of the linear equation characterizing the behaviour of the 
 * projectile's trajectory. Determined at the creation of the projectile_t object.
 */
typedef struct {
    float slope;
    int32_t offset;
    uint32_t timer;
    physics_t physics;
} projectile_t;

/**
 * @brief Struct handling the properties of a platform.
 */
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


/*************************************************
 * External variables
 *************************************************/

/**
 * Those arrays are used to store in memory the different object types
 * generated in-game.
*/
extern block_t blocks[NUM_BLOCK_RECORDS];
extern item_t items[NUM_ITEMS];
extern enemy_t enemies[NUM_ENEMY_RECORDS];
extern projectile_t projectiles[MAX_PROJECTILES];
extern platform_t platforms[MAX_PLATFORMS];


/*************************************************
 * Function prototypes (blocks/items)
 *************************************************/

/**
 * @brief Initialize all block records. 
 */
void initialize_blocks_records(void);

/**
 * @brief Search and retrieve the desired block record index using the block's row
 * and column. The block record can then be accessed using the block index on blocks[].
 * 
 * @param[out] index Pointer to the block record index.
 * @param[in] row Row of the block in the map.
 * @param[in] column Column of the block in the map.
 * 
 * @return 1 if the block record has been found, else 0.
 */
uint8_t get_block_record(uint8_t *index, const int16_t row, const int8_t column);

/**
 * @brief Check if the block is destroyed.
 * 
 * @param[in] row Row of the block in the map.
 * @param[in] column Column of the block in the map.
 * 
 * @return 1 if the block destroyed, else 0.
 */
uint8_t is_block_destroyed(const int16_t row, const int8_t column);

/**
 * @brief Check for collisions between the physics_t object and its surrounding
 * blocks.
 * 
 * @param[in] map Pointer to the current game map.
 * @param[in] physics Pointer to the physics_t object to check for collisions.
 * @param[in] cam_row Row at which the camera is positioned.
 * 
 * @return 1 if collision, else 0.
 * 
 * @warning The position of the collision is taken from the reference of the given
 * physical object. Hence, a 'left' collision means that the object has a collision on its
 * left side.
 */
uint8_t check_block_collisions(const map_t *map, physics_t *physics, const uint16_t cam_row);

/**
 * @brief Update the position of the given physics_t object, simulating reactive 
 * forces applied by one or several collisions with adjacent blocks.
 * 
 * @param physics Pointer to the physics_t object.
 */
void apply_reactive_force(physics_t *physics);

/**
 * @brief Animate the block by making it do a little bump in the air.
 * 
 * @param block Pointer to the block to animate.
 * @param sprite Pointer to the sprite of the block to animate.
 * @param timer Game timer for proper animation timing.
 * 
 * @warning The sprite and block pointers shall correspond, else the function
 * will not work as desired.
 */
void bump_block(block_t *block, sprite_t *sprite, const uint64_t timer);

/**
 * @brief Store a given item in memory.
 * 
 * @param item Pointer to the item to store in memory.
 */
void store_item(item_t *item);


/*************************************************
 * Function prototypes (player/enemies)
 *************************************************/

/**
 * @brief Spawn all enemies exisiting over the given range, between start_row and end_row.
 * The function will not spawn an enemy that has already been spawned.
 * 
 * @param map Pointer to the current game map.
 * @param cam_pos_x x-position of the camera in pixels.
 * @param start_row Row at which to start spawning enemies.
 * @param end_row Row at which to end spawning enemies (excluded).
 * 
 * @warning @p end_row is excluded from the range at which enemies are being spawned. This means enemies
 * are being spawned until (end_row - 1) included.
 */
void spawn_enemies(const map_t *map, const uint16_t cam_pos_x, const int16_t start_row, const int16_t end_row);

/**
 * @brief Set the state of the physics_t object to jumping.
 * 
 * @param physics Pointer to the physics_t object.
 * @param initial_speed Initial speed of the jump. Commonly known as v0.
 */
void initiate_jump(physics_t *physics, const uint8_t initial_speed);

/**
 * @brief Check if the shooter has the target on sight.
 * 
 * @param map Pointer to the current game map.
 * @param shooter Pointer to the physics_t object being the shooter.
 * @param target Pointer to the physics_t object being the target.
 * 
 * @return uint8_t 1 if on sight, else 0.
 * 
 * @note On sight means that the target is in range and that the path between
 * the shooter and the target is clear of solid blocks.
 */
uint8_t is_on_sight(const map_t *map, physics_t *shooter, physics_t *target);

/**
 * @brief Create, configure and store a projectile_t object in memory.
 * 
 * @param shooter Pointer to the physics_t object being the shooter.
 * @param target Pointer to the physics_t object being the target.
 */
void shoot_projectile(const physics_t *shooter, const physics_t *target);


/*************************************************
 * Function prototypes (platforms)
 *************************************************/

/**
 * @brief Check if the given platform exists in memory and retrieve its index
 * if found.
 * 
 * @param index Pointer to the platform's index.
 * @param row Row at which the platform is currently positioned.
 * @param column Column at which the platform currently is positioned.
 * 
 * @return 1 if the platform has been successfully retrieved, else 0.
 * 
 * @note To check if the platform is loaded in memory without retrieving its
 * index, it is possible to pass a NULL pointer as input. In this case, the function will
 * not try to dereference the pointer.
 */
uint8_t get_platform(uint8_t *index, const int16_t row, const int8_t column);

/**
 * @brief Create, configure and load all of the map's platforms into memory.
 * 
 * @param map Pointer to the current game map.
 */
void load_platforms(const map_t *map);

/**
 * @brief Check for a collision between the physics_t object and the platforms
 * in memory.
 * 
 * @param physics Pointer to the physics_t object.
 * @return 1 if collision found, else 0.
 */
uint8_t check_platform_collision(physics_t *physics);


#endif // __GAME_ENGINE_H__