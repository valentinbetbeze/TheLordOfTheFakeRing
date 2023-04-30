#include "game_engine.h"

/**
 * @brief 
 * 
 */
static block_state_t block_record[NUM_EVENTS];


/**
 * @brief Get the block id object
 * 
 * @param row 
 * @param column 
 * @return uint16_t 
 */
static uint16_t get_block_id(uint16_t row, uint16_t column)
{
    if (row == 0 || NB_BLOCKS_Y <= column) {
        return U12BIT(-1);
    }
    return (uint16_t)(row - 1) * NB_BLOCKS_Y + column;
}


/**
 * @brief Get the block row object
 * 
 * @param id 
 * @return uint16_t 
 */
static uint16_t get_block_row(uint16_t id)
{
    return (uint16_t) id / NB_BLOCKS_Y + 1;
}


/**
 * @brief Get the block column object
 * 
 * @param id 
 * @return uint16_t 
 */
static uint16_t get_block_column(uint16_t id)
{
    return (uint16_t) id % NB_BLOCKS_Y;
}


/**
 * @brief 
 * 
 * @param id 
 * @param block_type 
 * @param map_row 
 * @return uint8_t 
 */
static uint8_t update_block_state(uint16_t id, int8_t block_type, uint16_t map_row)
{
    // Initialize the block_record array the first time only
    static uint8_t _initialized_ = 0;
    if (!_initialized_) {
        for (int i = 0; i < NUM_EVENTS; i++) {
            block_record[i].id           = U12BIT(-1); // Empty slot identifier
            block_record[i].hit          = 0;
            block_record[i].destroyed    = 0;
            block_record[i].has_item     = 0;
        }
        _initialized_ = 1;
    }
    // Abort if the block cannot interact
    if (block_type < BREAKABLE_BLOCK) {
        return 1;
    }
    // Scan the block_record to find or create the state record of the block
    int8_t index = -1;
    uint8_t free_slot_found = 0;
    for (int8_t i = 0; i < NUM_EVENTS; i++) {
        if (block_record[i].id == id) {
            index = i;
            break;
        }
        else if (!free_slot_found && block_record[i].id == U12BIT(-1)) {
            index = i;
            free_slot_found = 1;
        }
        // If the block is not in the frame anymore, write over it
        else if (!free_slot_found && (get_block_row(block_record[i].id) < map_row)) {
            index = i;
        }
    }
    if (index == -1) {
        printf("Error(update_block_state): Cannot add new event (block_record full).\n");
        return 1;
    }
    // Set the state of the block
    switch (block_type) {
        case BREAKABLE_BLOCK:
            block_record[index].id = id;
            block_record[index].hit = 1;
            block_record[index].destroyed = 1;
            block_record[index].has_item = 0;
            break;
        case BONUS_BLOCK:
            block_record[index].id = id;
            block_record[index].hit = 1;
            block_record[index].destroyed = 0;
            block_record[index].has_item = 0;
            break;
        default:
            return 1;
    }
    return 0;
}

/**
 * @brief 
 * 
 * @param id 
 * @return uint8_t 
 */
static uint8_t is_block_destroyed(uint16_t id)
{
    block_state_t *block_state = get_block_state(id);
    if (block_state != NULL && block_state->destroyed) {
        return 1;
    }
    return 0;
}


block_state_t *get_block_state(uint16_t id)
{
    for (int i = 0; i < NUM_EVENTS; i++) {
        if (block_record[i].id == id) {
            return &block_record[i];
        }
    }
    return NULL; // Block state not found
}


int8_t check_collisions(const int8_t map[][NB_BLOCKS_Y], character_t *character, uint16_t map_x)
{   
    if (map == NULL) {
        printf("Error(check_block_collisions): Map file does not exist.\n");
        return -1;
    }
    if (character == NULL) {
        printf("Error(check_block_collisions): Character cannot be found.\n");
        return -1;
    }
    if (character->speed_x * 2 >= BLOCK_SIZE || character->speed_y * 2 >= BLOCK_SIZE) {
        printf("Error(check_block_collisions): Speed is too high. Max speed = (BLOCK_SIZE / 2) - 1\n");
        return -1;
    }
    // Reset character collision states
    character->top_collision    = 0;
    character->bottom_collision = 0;
    character->left_collision   = 0;
    character->right_collision  = 0;
    // Check if the character is within any potential collision area
    uint8_t x_offset = (character->sprite.pos_x + map_x) % BLOCK_SIZE;
    uint8_t y_offset = character->sprite.pos_y % BLOCK_SIZE;
    struct {
        uint8_t left :      1;
        uint8_t right :     1;
        uint8_t top :       1;
        uint8_t bottom :    1;
    } collision_risk = {
        .left   = ((uint8_t)BLOCK_SIZE - character->speed_x <= x_offset),
        .right  = (1 <= x_offset && x_offset <= character->speed_x),
        .top    = ((uint8_t)BLOCK_SIZE / 2 < y_offset && y_offset <= BLOCK_SIZE - 1),
        .bottom = (1 <= y_offset && y_offset < (uint8_t)BLOCK_SIZE / 2),
    };
    // If no collision risk, abort
    if (!collision_risk.left && !collision_risk.right && !collision_risk.top && !collision_risk.bottom) {
        return 0;
    }
    // Compute reference parameters
    uint16_t map_row = (uint16_t)(map_x / BLOCK_SIZE) + 1;
    uint16_t row_character = (uint16_t)(character->sprite.pos_x + map_x) / BLOCK_SIZE + 1;
    uint8_t col_character = NB_BLOCKS_Y - (uint8_t)(character->sprite.pos_y / BLOCK_SIZE) - 1;
    int8_t collisions = 0;
    uint16_t block_id = 0;
    // Get all 4 adjacent block types
    int8_t block_tl = map[row_character][col_character];           // top-left block
    int8_t block_tr = map[row_character + 1][col_character];       // top-right block
    int8_t block_bl = map[row_character][col_character - 1];       // bottom-left block
    int8_t block_br = map[row_character + 1][col_character - 1];   // bottom-right block
    uint8_t block_tl_id = get_block_id(row_character, col_character);
    uint8_t block_tr_id = get_block_id(row_character + 1, col_character);
    uint8_t block_bl_id = get_block_id(row_character, col_character - 1);
    uint8_t block_br_id = get_block_id(row_character + 1, col_character - 1);
    // Check left/right collision
    if ((collision_risk.left && IS_SOLID(block_tl) && !is_block_destroyed(block_tl_id)) ||
        (collision_risk.left && y_offset && IS_SOLID(block_bl) && !is_block_destroyed(block_bl_id) &&
        (!IS_SOLID(block_br) || is_block_destroyed(block_br_id)))) {

        character->left_collision = 1;
        collisions++;
    }
    else if ((collision_risk.right && IS_SOLID(block_tr) && !is_block_destroyed(block_tr_id)) ||
             (collision_risk.right && y_offset && IS_SOLID(block_br) && !is_block_destroyed(block_br_id) &&
             (!IS_SOLID(block_bl) || is_block_destroyed(block_bl_id)))) {
             
        character->right_collision = 1;
        collisions++;
    }
    // Check top/bottom collision
    if ((collision_risk.top && IS_SOLID(block_tl) && !is_block_destroyed(block_tl_id) &&
        x_offset < BLOCK_SIZE - character->speed_x) ||
        (collision_risk.top &&  character->speed_x < x_offset && IS_SOLID(block_tr) &&
        !is_block_destroyed(block_tr_id))) {

        character->top_collision = 1;
        collisions++;
        if (x_offset <= BLOCK_SIZE / 2 && IS_SOLID(block_tl) && !is_block_destroyed(block_tl_id)) {
            block_id = get_block_id(row_character, col_character);
            update_block_state(block_id, block_tl, map_row);
        }
        else if (x_offset > BLOCK_SIZE / 2 && IS_SOLID(block_tr) && !is_block_destroyed(block_tr_id)) {
            block_id = get_block_id(row_character + 1, col_character);
            update_block_state(block_id, block_tr, map_row);
        }
    }
    else if ((collision_risk.bottom && IS_SOLID(block_bl) && !is_block_destroyed(block_bl_id) &&
            x_offset < BLOCK_SIZE - character->speed_x) || 
            (collision_risk.bottom && character->speed_x < x_offset && IS_SOLID(block_br) &&
            !is_block_destroyed(block_br_id))) {
        character->bottom_collision = 1;
        collisions++;
    }
    // Apply collision's reactive force
    if (character->left_collision) {
        character->sprite.pos_x += BLOCK_SIZE - x_offset;
    }
    else if (character->right_collision) {
        character->sprite.pos_x -= x_offset;
    }
    if (character->top_collision) {
        character->sprite.pos_y += BLOCK_SIZE - y_offset;
    }
    else if (character->bottom_collision) {
        character->sprite.pos_y -= y_offset;
    }

    return collisions;
}


uint8_t build_frame(const int8_t map[][NB_BLOCKS_Y], uint16_t map_x)
{
    if (map == NULL) {
        printf("Error(build_frame): Map file does not exist.\n");
        return 1;
    }
    st7735s_fill_background(MAP_BACKGROUND(map));
    // Iterate through each block of the current map frame
    uint16_t map_row = (uint16_t)(map_x / BLOCK_SIZE) + 1;
    for (int i = map_row; i <= map_row + NB_BLOCKS_X; i++) {
        for (int j = 0; j < NB_BLOCKS_Y; j++) {
            // If background block, go to next block
            int8_t block_type = map[i][NB_BLOCKS_Y - j - 1];
            if (block_type == BACKGROUND_BLOCK) {
                continue;
            }
            // If the block has been destroyed, go to next block
            block_state_t *block_state = get_block_state(get_block_id(i, NB_BLOCKS_Y - j - 1));
            if (block_state != NULL && block_state->destroyed) {
                continue;
            }
            // Draw the block
            uint8_t map_id = MAP_ID(map);
            sprite_t block = {
                .height = BLOCK_SIZE,
                .width = BLOCK_SIZE,
                .pos_x = (i - 1) * BLOCK_SIZE - map_x,
                .pos_y = j * BLOCK_SIZE
            };
            switch (block_type) {
                case NON_BREAKABLE_BLOCK:
                    switch (map_id) {
                        case 1: block.data = shire_block_1; break;
                        case 2: block.data = moria_block_1; break;
                        default: break;
                    }
                    break;
                case BREAKABLE_BLOCK:
                    switch (map_id) {
                        case 1: block.data = shire_block_2; break;
                        case 2: block.data = moria_block_2; break;
                        default: break;
                    }
                    break;
                case BONUS_BLOCK:
                    switch (map_id) {
                        case 2: block.data = moria_block_3; break;
                        default: break;
                    }
                    break;
                default: 
                    break;
            }
            st7735s_draw_sprite(block);
        }
    }
    return 0;
}
