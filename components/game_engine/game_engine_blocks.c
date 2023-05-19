#include "game_engine.h"

#define TIMESTEP_BUMP_BLOCK     5           // in milliseconds
#define HEIGHT_BUMP_BLOCK       3           // Bump height of a block, in pixels

/**
 * @warning Do not initialize to {0}: blocks[] has its own initialization
 * function as some parameters shall be set at specific values before
 * the program runs. See initialize_blocks_records().
 */
block_t blocks[NUM_BLOCK_RECORDS]; 
item_t items[NUM_ITEMS] = {0};

/**
 * @brief Create a block record and store it in memory.
 * 
 * @param row Row of the block from the map.data array.
 * @param column Column of the block from the map.data array.
 * @param cam_row Row at which the camera is positioned.
 * 
 * @return 1 if the record is successfully created, 0 if a record already exists
 * for the given block.
 * 
 * @note The block is identified thanks to its unique row/column combinaison.
 */
static uint8_t create_block_record(const int16_t row, const int8_t column, const uint16_t cam_row)
{
    // Scan the blocks log to create the state record of the block
    int8_t index = -1;
    for (int8_t i = 0; i < NUM_BLOCK_RECORDS; i++) {
        // Record already exists
        if (blocks[i].row == row && blocks[i].column == column) {
            return 0;
        }
        // Empty slot found
        else if (blocks[i].row == -1 && blocks[i].column == -1) {
            index = i;
            break;
        }
        // The block is not in the frame anymore
        else if (blocks[i].row < cam_row) {
            index = i;
        }
    }
    if (index == -1) {
        printf("Error(create_block_record): Cannot add new record (blocks[] full).\n");
        assert(index != -1);
    }
    // Create the block record
    memset(&blocks[index], 0, sizeof(blocks[index]));        
    blocks[index].row       = row;
    blocks[index].column    = column;
    blocks[index].is_hit    = 1;
    return 1;
}


/**
 * @brief Set the given block state as hit.
 * 
 * @param row Row of the block from the map.data array.
 * @param column Column of the block from the map.data array.
 * @param cam_row Row at which the camera is positioned.
 */
static void set_block_as_hit(const int16_t row, const int8_t column, const uint16_t cam_row)
{
    // Create a new record for the block
    if (!create_block_record(row, column, cam_row)) {
        // If a record already exists, update the current one.
        uint8_t index;
        if (!get_block_record(&index, row, column)) {
            printf("Error(set_block_as_hit): Conflict: create_block_record() found the block record but get_block_record() could not.\n");
            assert(0);
        }
        blocks[index].is_hit = 1;
    }
}


void initialize_blocks_records(void)
{
    for (int i = 0; i < NUM_BLOCK_RECORDS; i++) {
        blocks[i].row          = -1; // Empty slot identifier
        blocks[i].column       = -1;
        blocks[i].is_hit       = 0;
        blocks[i].destroyed    = 0;
        blocks[i].item_given   = 0;
        blocks[i].bumping      = 0;
    }
}


uint8_t get_block_record(uint8_t *index, const int16_t row, const int8_t column)
{
    if (index == NULL) {
        printf("Error(get_block_record): Pointer `index` is NULL.\n");
        assert(index);
    }
    for (int8_t i = 0; i < NUM_BLOCK_RECORDS; i++) {
        if (blocks[i].row == row && blocks[i].column == column) {
            *index = i;
            return 1;
        }
    }
    return 0; // Block not found
}


uint8_t is_block_destroyed(const int16_t row, const int8_t column)
{
    uint8_t index;
    if (get_block_record(&index, row, column) && blocks[index].destroyed) {
        return 1;
    }
    return 0;
}


uint8_t check_block_collisions(const map_t *map, physics_t *physics, const uint16_t cam_row)
{   
    if (map == NULL) {
        printf("Error(check_block_collisions): map_t pointer is NULL.\n");
        assert(map);
    }
    if (map->data == NULL) {
        printf("Error(check_block_collisions): Map data pointer is NULL.\n");
        assert(map->data);
    }
    if (physics == NULL) {
        printf("Error(check_block_collisions): physics_t pointer is NULL.\n");
        assert(physics);
    }
    if (physics->speed_x * 2 >= BLOCK_SIZE || physics->speed_y * 2 >= BLOCK_SIZE) {
        printf("Warning(check_block_collisions): Speed is too high. Max speed = (BLOCK_SIZE / 2) - 1\n");
        assert(physics->speed_x * 2 < BLOCK_SIZE && physics->speed_y * 2 < BLOCK_SIZE);
    }

    // Reset collision states
    physics->top_collision    = 0;
    physics->bottom_collision = 0;
    physics->left_collision   = 0;
    physics->right_collision  = 0;

    // Compute reference parameters
    const int16_t ref_row = (int16_t)physics->pos_x / BLOCK_SIZE;
    int8_t ref_col, block_tl, block_tr, block_bl, block_br;
    if (physics->pos_y < 0) {
        ref_col = 8;
        block_bl = map->data[ref_row][ref_col - 1];
        block_br = map->data[ref_row + 1][ref_col - 1];
        if (IS_SOLID(block_bl) && !is_block_destroyed(ref_row, ref_col - 1)) block_tl = NON_BREAKABLE_BLOCK_1;
        else block_tl = BACKGROUND_BLOCK;
        if (IS_SOLID(block_br) && !is_block_destroyed(ref_row + 1, ref_col - 1)) block_tr = NON_BREAKABLE_BLOCK_1;
        else block_tr = BACKGROUND_BLOCK;
    }
    else {
        ref_col = NUM_BLOCKS_Y - (int8_t)(physics->pos_y / BLOCK_SIZE) - 1;
        block_tl = map->data[ref_row][ref_col];           // top-left block
        block_tr = map->data[ref_row + 1][ref_col];       // top-right block
        block_bl = map->data[ref_row][ref_col - 1];       // bottom-left block
        block_br = map->data[ref_row + 1][ref_col - 1];   // bottom-right block    
    }

    // Check if the object is within any potential collision area
    struct {
        uint8_t left :      1;
        uint8_t right :     1;
        uint8_t top :       1;
        uint8_t bottom :    1;
    } collision_risk;
    const uint8_t x_offset = physics->pos_x % BLOCK_SIZE;
    const uint8_t y_offset = physics->pos_y % BLOCK_SIZE;
    collision_risk.top = (BLOCK_SIZE / 2 < y_offset) && (y_offset <= BLOCK_SIZE - 1);
    collision_risk.bottom = (1 <= y_offset) && (y_offset < (uint8_t)BLOCK_SIZE / 2) &&
                            (physics->pos_y < LCD_HEIGHT - BLOCK_SIZE);
    if ((physics->jumping && IS_SOLID(block_tl) && !is_block_destroyed(ref_row, ref_col)) ||
        (physics->falling && IS_SOLID(block_br) && !is_block_destroyed(ref_row + 1, ref_col - 1))) {
        collision_risk.right = (1 <= x_offset) && (x_offset <= abs(physics->speed_x));
    }
    else {
        collision_risk.right = (1 <= x_offset) && (x_offset <= abs(physics->speed_x) + SLIP_OFFSET);
    }
    if ((physics->jumping && IS_SOLID(block_tr) && !is_block_destroyed(ref_row + 1, ref_col)) || 
        (physics->falling && IS_SOLID(block_bl) && !is_block_destroyed(ref_row, ref_col - 1))) {
        collision_risk.left = BLOCK_SIZE - abs(physics->speed_x) <= x_offset;
    }
    else {
        collision_risk.left = BLOCK_SIZE - abs(physics->speed_x) - SLIP_OFFSET <= x_offset;
    }
    // If no collision risk, abort
    if (!collision_risk.left && !collision_risk.right && !collision_risk.top && !collision_risk.bottom) {
        return 0;
    }

    // Check for collisions
    physics->top_collision =        (collision_risk.top && IS_SOLID(block_tl) && !is_block_destroyed(ref_row, ref_col)
                                    && x_offset < BLOCK_SIZE - abs(physics->speed_x) - SLIP_OFFSET) ||
                                    (collision_risk.top && abs(physics->speed_x) + SLIP_OFFSET < x_offset &&
                                    IS_SOLID(block_tr) && !is_block_destroyed(ref_row + 1, ref_col));

    physics->bottom_collision =     (collision_risk.bottom && IS_SOLID(block_bl) && !is_block_destroyed(ref_row, ref_col - 1)
                                    && x_offset < BLOCK_SIZE - abs(physics->speed_x)) ||
                                    (collision_risk.bottom && abs(physics->speed_x) < x_offset &&
                                    IS_SOLID(block_br) && !is_block_destroyed(ref_row + 1, ref_col - 1));

    physics->left_collision =   (collision_risk.left && IS_SOLID(block_tl) && !is_block_destroyed(ref_row, ref_col));
    physics->right_collision =  (collision_risk.right && IS_SOLID(block_tr) && !is_block_destroyed(ref_row + 1, ref_col));
                                
    if (physics->jumping || physics->falling) {
        physics->left_collision |=  (collision_risk.left && y_offset && IS_SOLID(block_bl) && 
                                    !is_block_destroyed(ref_row, ref_col - 1) &&
                                    (!IS_SOLID(block_br) || is_block_destroyed(ref_row + 1, ref_col - 1)));

        physics->right_collision |= (collision_risk.right && y_offset && IS_SOLID(block_br) && 
                                    !is_block_destroyed(ref_row + 1, ref_col - 1) &&
                                    (!IS_SOLID(block_bl) || is_block_destroyed(ref_row, ref_col - 1)));
    }
    else if (physics->grounded) {
        physics->left_collision |=  ((IS_SOLID(block_br) && !is_block_destroyed(ref_row + 1, ref_col - 1)) &&
                                    (!IS_SOLID(block_bl) || is_block_destroyed(ref_row, ref_col - 1)) && x_offset);

        physics->right_collision |= ((IS_SOLID(block_bl) && !is_block_destroyed(ref_row, ref_col - 1)) &&
                                    (!IS_SOLID(block_br) || is_block_destroyed(ref_row + 1, ref_col - 1)) && x_offset);
    }
    // Change interactive blocks state if appropriate
    if (physics->top_collision && IS_INTERACTIVE(block_tl) && !is_block_destroyed(ref_row, ref_col) &&
        x_offset <= BLOCK_SIZE / 2) {
            set_block_as_hit(ref_row, ref_col, cam_row);
        }
    else if (physics->top_collision && IS_INTERACTIVE(block_tr) && !is_block_destroyed(ref_row + 1, ref_col) &&
             BLOCK_SIZE / 2 < x_offset) {
        set_block_as_hit(ref_row + 1, ref_col, cam_row);
    }
    return (physics->top_collision || physics->bottom_collision || physics->left_collision || physics->right_collision);
}


void apply_reactive_force(physics_t *physics)
{
    if (physics == NULL) {
        printf("Error(fix_block_collision): physics_t pointer is NULL.\n");
        assert(physics);
    }
    const uint8_t x_offset = physics->pos_x % BLOCK_SIZE;
    const uint8_t y_offset = physics->pos_y % BLOCK_SIZE;
    // x-direction
    if (physics->left_collision && physics->platform_i == -1) {
        physics->pos_x += BLOCK_SIZE - x_offset;
    }
    else if (physics->right_collision && physics->platform_i == -1) {
        physics->pos_x -= x_offset;
    }
    // y-direction
    if (physics->top_collision) {
        physics->pos_y += BLOCK_SIZE - y_offset;
    }
    else if (physics->bottom_collision) {
        physics->pos_y -= y_offset;
    }
}


void bump_block(block_t *block, sprite_t *sprite, const uint64_t timer)
{
    if (block == NULL) {
        printf("Error(bump_block): block_t pointer is NULL.\n");
        assert(block);
    }
    if (sprite == NULL) {
        printf("Error(bump_block): sprite_t pointer is NULL.\n");
        assert(sprite);
    }

    static uint64_t t0 = 0;
    static uint8_t steps = 0;
    if ((timer - t0) / TIMESTEP_BUMP_BLOCK < 1) {
        return;
    }

    steps++;
    if (steps <= HEIGHT_BUMP_BLOCK) {
        sprite->pos_y -= steps;
    }
    else if (steps <= HEIGHT_BUMP_BLOCK * 2) {
        sprite->pos_y -= (HEIGHT_BUMP_BLOCK * 2 - steps);
    }
    else {
        block->bumping = 0;
        steps = 0;
    }
    t0 = timer;
}


void store_item(item_t *item)
{
    if (item == NULL) {
        printf("Error(store_item): item_t pointer is NULL.\n");
        assert(item);
    }
    // Scan the item log to store the item
    int8_t index = -1;
    for (int8_t i = 0; i < NUM_ITEMS; i++) {
        if (!items[i].spawned || items[i].taken || items[i].sprite.pos_x < -BLOCK_SIZE) {
            index = i;
            break;
        }
    }
    if (index == -1) {
        printf("Error(store_item): Cannot add new item (items[] full).\n");
        assert(0);
    }
    memset(&items[index], 0, sizeof(items[index]));
    items[index] = *item;
}
