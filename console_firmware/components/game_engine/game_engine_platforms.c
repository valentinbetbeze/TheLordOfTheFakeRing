#include "game_engine.h"

platform_t platforms[MAX_PLATFORMS] = {0};


/**
 * @brief Configure a platform. The function scans the map and sets the
 * members of the platform_t variable to reflect the platform's path. 
 * 
 * @param map Pointer to the current game map.
 * @param platform Pointer to the platform_t object to configure.
 */
static void configure_platform(const map_t *map, platform_t *platform)
{
    if (map == NULL) {
        printf("Error(configure_platform): map_t pointer is NULL.\n");
        assert(map);
    }
    if (map->data == NULL) {
        printf("Error(configure_platform): Map data pointer is NULL.\n");
        assert(map->data);
    }
    platform->physics.platform_i = -1;
    platform->end_row = platform->start_row;
    platform->end_column = platform->start_column;
    block_type_t block_right = 0;
    block_type_t block_top = 0;
    // Determine the trajectory of the platform
    if (platform->start_row < map->nrows - 1) {
        block_right = map->data[platform->start_row + 1][platform->start_column];
    }
    if (platform->start_column < map->ncolumns - 1) {
        block_top = map->data[platform->start_row][platform->start_column + 1];
    }
    if (block_right == PLATFORM_BLOCK && block_top != PLATFORM_BLOCK) {
        // Horizontal platform, keep searching to the right
        platform->horizontal = 1;
        platform->physics.speed_x = SPEED_PLATFORM;
        while (map->data[platform->end_row + 1][platform->start_column] == PLATFORM_BLOCK) {
            platform->end_row++;
            if (map->nrows - 1 <= platform->end_row) {
                break;
            }
        }
    }
    else if (block_right != PLATFORM_BLOCK && block_top == PLATFORM_BLOCK) {
        // Vertical platform, keep searching to the top
        platform->vertical = 1;
        platform->physics.speed_y = SPEED_PLATFORM;
        while (map->data[platform->start_row][platform->end_column + 1] == PLATFORM_BLOCK) {
            platform->end_column++;
            if (map->ncolumns - 1 <= platform->end_column) {
                break;
            }
        }
    }
    else if (block_right != PLATFORM_BLOCK && block_top != PLATFORM_BLOCK) {
        static uint8_t configure_platform_flag = 0;
        if (!configure_platform_flag) {
            printf("Warning(configure_platform): Platform at row = %i, column = %i, has no trajectory.\n", platform->start_row, platform->start_column);
        }
    }
    else {
        printf("Error(configure_platform): Platform at row = %i, column = %i, has an undetermined trajectory. Please fix the trajectory in map->c, map id: %i\n", platform->start_row, platform->start_column, map->id);
        assert(0);
    }
}


/**
 * @brief 
 * 
 * @param game 
 * @param index 
 */
static void update_platform_x(const game_t *game, platform_t *platform)
{
    // The platform has reach the end of its course
    if ((platform->physics.pos_x < platform->start_row * BLOCK_SIZE) ||
        (platform->end_row * BLOCK_SIZE <= platform->physics.pos_x)) {

        platform->physics.speed_x *= -1;
        platform->changed_dir = 1;
    }
    // Update x-position
    platform->physics.pos_x += platform->physics.speed_x;
    platform->timer = (uint32_t)game->timer;
    platform->moved = 1;
}


/**
 * @brief 
 * 
 * @param game 
 * @param platform
 */
static void update_platform_y(const game_t *game, platform_t *platform)
{
    const uint8_t start_y = LCD_HEIGHT - (platform->start_column + 1) * BLOCK_SIZE;
    const uint8_t end_y = LCD_HEIGHT - (platform->end_column + 1) * BLOCK_SIZE;
    // The platform has reach the end of its course
    if (start_y < platform->physics.pos_y || platform->physics.pos_y <= end_y) {
        platform->physics.speed_y *= -1;
        platform->changed_dir = 1;
    }
    // Update y-position
    platform->physics.pos_y += platform->physics.speed_y;
    platform->timer = (uint32_t)game->timer;
    platform->moved = 1;
}


uint8_t get_platform(uint8_t *index, const int16_t row, const int8_t column)
{
    if (row == -1 || column == -1) {
        return 0;
    }
    for (int8_t i = 0; i < MAX_PLATFORMS; i++) {
        if (platforms[i].start_row <= row && row <= platforms[i].end_row &&
            platforms[i].start_column <= column && column <= platforms[i].end_column) {
            if (index != NULL) {
                *index = i;
            }
            return 1;
        }
    }
    return 0; // Platform not found
}


void load_platforms(const map_t *map)
{
    if (map == NULL) {
        printf("Error(load_platforms): map_t pointer is NULL.\n");
        assert(map);
    }
    if (map->data == NULL) {
        printf("Error(load_platforms): Map data pointer is NULL.\n");
        assert(map->data);
    }
    // Scan the map to find and create platforms
    uint8_t index_platform = 0;
    for (uint16_t row = 0; row < map->nrows; row++) {
        for (uint8_t column = 0; column < NUM_BLOCKS_Y; column++) {
            // A platform has been found
            if (map->data[row][column] == PLATFORM_BLOCK) {
                if (MAX_PLATFORMS <= index_platform) {
                    printf("Error(load_platforms): Too many platforms in map: id = %i. Remove some platforms or increase MAX_PLATFORMS.\n", map->id);
                    assert(0);
                }
                else if (get_platform(NULL, row, column)) {
                    // Platform already exists
                    continue;
                }
                // Create the platform & store it in memory
                platform_t platform = {
                    .start_row = row,
                    .start_column = column,
                    .physics.pos_x = row * BLOCK_SIZE,
                    .physics.pos_y = LCD_HEIGHT - (column + 1) * BLOCK_SIZE,
                };
                configure_platform(map, &platform);
                platforms[index_platform] = platform;
                index_platform++;
            }
        }
    }
}


void update_platform_position(const game_t *game, platform_t *platform)
{
    if (game == NULL) {
        printf("Error(update_platform_position): game_t pointer is NULL.\n");
        assert(game);
    }
    if (platform == NULL) {
        printf("Error(update_platform_position): platform_t pointer is NULL.\n");
        assert(platform);
    }
    // Abort if no more platforms to update
    platform->moved = 0;
    platform->changed_dir = 0;
    if (platform->start_row == -1) {
        return;
    }
    else if (platform->vertical &&
            (game->timer - platform->timer) / TIMESTEP_PLATFORM > 1) {
        update_platform_y(game, platform);
    }
    else if (platform->horizontal &&
            (game->timer - platform->timer) / TIMESTEP_PLATFORM > 1) {
        update_platform_x(game, platform);
    }
}


uint8_t check_platform_collision(physics_t *physics, platform_t *platform)
{
    if (physics == NULL) {
        printf("Error(check_platform_collision): physics_t pointer is NULL.\n");
        assert(physics);
    }
    if (platform == NULL) {
        printf("Error(check_platform_collision): platform_t pointer is NULL.\n");
        assert(platform);
    }
    uint8_t on_platform_x, on_hplatform_y, on_vplatform_y;
    on_platform_x = (platform->physics.pos_x - BLOCK_SIZE + 2 < physics->pos_x) &&
                    (physics->pos_x < platform->physics.pos_x + 2 * BLOCK_SIZE - 2);

    on_hplatform_y =    (platform->physics.pos_y - BLOCK_SIZE < physics->pos_y) &&
                        (physics->pos_y <= platform->physics.pos_y - BLOCK_SIZE + physics->speed_y);
    
    on_vplatform_y = (
        (platform->moved && platform->physics.pos_y - BLOCK_SIZE <= physics->pos_y &&
        physics->pos_y <= platform->physics.pos_y - BLOCK_SIZE - platform->physics.speed_y + physics->speed_y) ||
        (!platform->moved && platform->physics.pos_y - BLOCK_SIZE < physics->pos_y &&
        physics->pos_y <= platform->physics.pos_y - BLOCK_SIZE + physics->speed_y));

    if ((platform->horizontal && on_platform_x && on_hplatform_y) ||
        (platform->vertical && on_platform_x && on_vplatform_y)) {
        // Collision found
        physics->bottom_collision = 1;
        if (physics->grounded) {
            physics->left_collision |= (physics->pos_x < platform->physics.pos_x);
            physics->right_collision |= (platform->physics.pos_x + BLOCK_SIZE < physics->pos_x);
        }
        return 1;
    }
    // No collision found
    physics->platform_i = -1;
    return 0;
}
