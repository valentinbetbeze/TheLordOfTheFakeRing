#include "game_engine.h"

static block_t blocks[NUM_BLOCK_RECORDS];
enemy_t enemies[NUM_ENEMY_RECORDS] = {0};


static uint8_t set_block_flags_on_hit(block_t *block, int8_t block_type)
{
    if (block == NULL) {
        printf("Error(set_block_flags_on_hit): Empty block_t pointer.\n");
        return 1;
    }
    switch (block_type) {
        case BREAKABLE_BLOCK:
            block->destroyed    = 1;
            block->has_item     = 0;
            block->bumping      = 0;
            break;
        case BONUS_BLOCK:
            block->destroyed    = 0;
            block->has_item     = 0;
            block->bumping      = 1;
            break;
        default:
            printf("Warning(set_block_flags_on_hit): You're trying to change the state of a block that cannot interact with external events. Block type: %i\n", block_type);
            break;
    }
    return 0;
}


static uint8_t is_block_destroyed(int16_t row, int8_t column)
{
    block_t *block = (block_t *)get_block_record(row, column);
    if (block != NULL && block->destroyed) {
        return 1;
    }
    return 0;
}


uint8_t create_block_record(block_t block, uint16_t map_row)
{
    // Initialize the blocks array the first time only
    static uint8_t bloc_record_initialized = 0;
    if (!bloc_record_initialized) {
        for (int i = 0; i < NUM_BLOCK_RECORDS; i++) {
            blocks[i].row          = -1; // Empty slot identifier
            blocks[i].column       = -1; // Empty slot identifier
            blocks[i].destroyed    = 0;
            blocks[i].has_item     = 0;
            blocks[i].bumping      = 0;
        }
        bloc_record_initialized = 1;
    }
    // Scan the blocks log to create the state record of the block
    int8_t index = -1;
    uint8_t free_slot_found = 0;
    for (int8_t i = 0; i < NUM_BLOCK_RECORDS; i++) {
        if (blocks[i].row == block.row && blocks[i].column == block.column) {
            return 1; // Record already exists
        }
        else if (!free_slot_found && blocks[i].row == -1 && blocks[i].column == -1) {
            index = i;
            free_slot_found = 1;
        }
        // If the block is not in the frame anymore, clean the slot and write over it
        else if (!free_slot_found && block.row < map_row) {
            memset(&blocks[index], 0, sizeof(blocks[index]));
            index = i;
        }
    }
    if (index == -1) {
        printf("Error(create_block_record): Cannot add new record (blocks[] full).\n");
        return 2;
    }
    blocks[index] = block;
    return 0;
}


block_t *get_block_record(int16_t row, int8_t column)
{
    for (int8_t i = 0; i < NUM_BLOCK_RECORDS; i++) {
        if (blocks[i].row == row && blocks[i].column == column) {
            return &blocks[i];
        }
    }
    return NULL; // Block not found
}


uint8_t check_block_collisions(const int8_t map[][NUM_BLOCKS_Y], physics_t *physics, uint16_t map_x)
{   
    if (map == NULL) {
        printf("Error(check_block_collisions): Map file does not exist.characterobjec\n");
        return 1;
    }
    if (physics == NULL) {
        printf("Error(check_block_collisions): Character cannot be found.\n");
        return 1;
    }
    if (physics->speed_x * 2 >= BLOCK_SIZE || physics->speed_y * 2 >= BLOCK_SIZE) {
        printf("Error(check_block_collisions): Speed is too high. Max speed = (BLOCK_SIZE / 2) - 1\n");
        return 1;
    }
    // Reset collision states
    physics->top_collision    = 0;
    physics->bottom_collision = 0;
    physics->left_collision   = 0;
    physics->right_collision  = 0;
    // Check if the object is within any potential collision area
    uint8_t x_offset = (physics->pos_x + map_x) % BLOCK_SIZE;
    uint8_t y_offset = physics->pos_y % BLOCK_SIZE;
    struct {
        uint8_t left :      1;
        uint8_t right :     1;
        uint8_t top :       1;
        uint8_t bottom :    1;
    } collision_risk = {
        .left   = ((uint8_t)BLOCK_SIZE - physics->speed_x <= x_offset),
        .right  = (1 <= x_offset && x_offset <= physics->speed_x),
        .top    = ((uint8_t)BLOCK_SIZE / 2 < y_offset && y_offset <= BLOCK_SIZE - 1),
        .bottom = (1 <= y_offset && y_offset < (uint8_t)BLOCK_SIZE / 2),
    };
    // If no collision risk, abort
    if (!collision_risk.left && !collision_risk.right && !collision_risk.top && !collision_risk.bottom) {
        return 0;
    }
    // Compute reference parameters
    uint16_t map_row = (uint16_t)(map_x / BLOCK_SIZE) + 1;
    int16_t ref_row = (int16_t)(physics->pos_x + map_x) / BLOCK_SIZE + 1;
    int8_t ref_col = NUM_BLOCKS_Y - (int8_t)(physics->pos_y / BLOCK_SIZE) - 1;
    // Get all 4 adjacent block types
    int8_t block_tl = map[ref_row][ref_col];           // top-left block
    int8_t block_tr = map[ref_row + 1][ref_col];       // top-right block
    int8_t block_bl = map[ref_row][ref_col - 1];       // bottom-left block
    int8_t block_br = map[ref_row + 1][ref_col - 1];   // bottom-right block
    // Check for collisions
    struct {
        uint8_t left :      1;
        uint8_t right :     1;
        uint8_t top :       1;
        uint8_t bottom :    1;
    } collision = {
        .left   = (collision_risk.left && IS_SOLID(block_tl) && 
                  !is_block_destroyed(ref_row, ref_col)) ||
                  (collision_risk.left && y_offset && IS_SOLID(block_bl) && 
                  !is_block_destroyed(ref_row, ref_col - 1) &&
                  (!IS_SOLID(block_br) || is_block_destroyed(ref_row + 1, ref_col - 1))),

        .right  = (collision_risk.right && IS_SOLID(block_tr) && 
                  !is_block_destroyed(ref_row + 1, ref_col)) ||
                  (collision_risk.right && y_offset && IS_SOLID(block_br) && 
                  !is_block_destroyed(ref_row + 1, ref_col - 1) &&
                  (!IS_SOLID(block_bl) || is_block_destroyed(ref_row, ref_col - 1))),

        .top    = (collision_risk.top && IS_SOLID(block_tl) &&
                  !is_block_destroyed(ref_row, ref_col) &&
                  x_offset < BLOCK_SIZE - physics->speed_x) ||
                  (collision_risk.top &&  physics->speed_x < x_offset && IS_SOLID(block_tr) &&
                  !is_block_destroyed(ref_row + 1, ref_col)),

        .bottom = (collision_risk.bottom && IS_SOLID(block_bl) && 
                  !is_block_destroyed(ref_row, ref_col - 1) &&
                  x_offset < BLOCK_SIZE - physics->speed_x) || 
                  (collision_risk.bottom && physics->speed_x < x_offset && IS_SOLID(block_br) &&
                  !is_block_destroyed(ref_row + 1, ref_col - 1)),
    };
    // Apply collisions
    if (collision.left) {
        physics->left_collision = 1;
        physics->pos_x += BLOCK_SIZE - x_offset;
    }
    else if (collision.right) {
        physics->right_collision = 1;
        physics->pos_x -= x_offset;
    }
    if (collision.top) {
        if (IS_INTERACTIVE(block_tl) && !is_block_destroyed(ref_row, ref_col)
            && x_offset <= BLOCK_SIZE / 2) {
            block_t block = {
                .row = ref_row,
                .column = ref_col
            };
            set_block_flags_on_hit(&block, block_tl);
            uint8_t err = create_block_record(block, map_row);
            if (err == 1) {
                block_t *record = get_block_record(ref_row, ref_col);
                *record = block;
                // TODO: modifying blockstates should be more modular. I have only
                // 1 function here to do it. Must be changed for object features.
                // TODO: Think about splitting this function if possible.
            }
            else if (err == 2) {
                return 1;
            }
        }
        else if (IS_INTERACTIVE(block_tr) && !is_block_destroyed(ref_row + 1, ref_col)
                 && BLOCK_SIZE / 2 < x_offset) {
            block_t block = {
                .row = ref_row + 1,
                .column = ref_col
            };
            set_block_flags_on_hit(&block, block_tr);
            uint8_t err = create_block_record(block, map_row);
            if (err == 1) {
                block_t *record = get_block_record(ref_row + 1, ref_col);
                *record = block;
            }
            else if (err == 2) {
                return 1;
            } 
        }
        physics->top_collision = 1;
        physics->pos_y += BLOCK_SIZE - y_offset;
    }
    else if (collision.bottom) {
        physics->bottom_collision = 1;
        physics->pos_y -= y_offset;
    }
    return 0;
}


void bump(block_t *block, sprite_t *sprite, uint64_t timer)
{
    static uint64_t t0 = 0;
    static uint8_t steps = 0;
    if ((int)(timer - t0 / TIMESTEP_BUMP) < 1) {
        return;
    }

    if (block == NULL) {
        printf("Error(bump): block pointer is NULL.\n");
        return;
    }
    if (sprite == NULL) {
        printf("Error(bump): sprite pointer is NULL.\n");
        return;
    }

    steps++;
    if (steps <= BUMP_HEIGHT) {
        sprite->pos_y -= steps;
    }
    else if (steps <= BUMP_HEIGHT * 2) {
        sprite->pos_y -= (BUMP_HEIGHT * 2 - steps);
    }
    else {
        block->bumping = 0;
        steps = 0;
    }
    t0 = timer;
}


void initialize_enemy(enemy_t *enemy, int16_t row, int8_t column, uint16_t map_row)
{
    enemy->row                      = row;
    enemy->column                   = column;
    enemy->life                     = 1;
    enemy->timer_x                  = 0;
    enemy->timer_y                  = 0;
    enemy->physics.top_collision    = 0;
    enemy->physics.bottom_collision = 0;
    enemy->physics.left_collision   = 0;
    enemy->physics.right_collision  = 0;
    enemy->physics.accelerating     = 0;
    enemy->physics.falling          = 0;
    enemy->physics.jumping          = 0;
    enemy->physics.pos_x            = BLOCK_SIZE * (row - map_row);
    enemy->physics.pos_y            = BLOCK_SIZE * (NUM_BLOCKS_Y - 1 - column);
    enemy->physics.speed_x          = INITIAL_SPEED;
    enemy->physics.speed_y          = INITIAL_SPEED;
}


uint8_t create_enemy_record(enemy_t enemy)
{
    // Scan the enemy log to create the state record of an enemy
    int8_t index = -1;
    uint8_t free_slot_found = 0;
    for (int8_t i = 0; i < NUM_ENEMY_RECORDS; i++) {
        if (enemies[i].row == enemy.row && enemies[i].column == enemy.column) {
            return 1; // Enemy record already exists
        }
        else if (!free_slot_found && enemies[i].life == 0) {
            index = i;
            free_slot_found = 1;
        }
        // If the enemy is not in the frame anymore, clean the slot and write over it
        else if (!free_slot_found && enemies[i].physics.pos_x < -BLOCK_SIZE) {
            index = i;
        }
    }
    if (index == -1) {
        printf("Error(update_enemy_state): Cannot add new enemy (enemies[] full).\n");
        return 2;
    }
    memset(&enemies[index], 0, sizeof(enemies[index]));
    enemies[index] = enemy;
    return 0;
}


enemy_t *get_enemy_record(int16_t row, int8_t column)
{
    for (int8_t i = 0; i < NUM_ENEMY_RECORDS; i++) {
        if (enemies[i].row == row && enemies[i].column == column) {
            return &enemies[i];
        }
    }
    return NULL; // Enemy not found
}


void spawn_enemies(const int8_t map[][NUM_BLOCKS_Y], int16_t start_row, int16_t end_row, uint16_t map_row)
{
    for (uint8_t row = start_row; row < end_row + 1; row++) {
        for (int column = 0; column < NUM_BLOCKS_Y; column++) {
            if (!IS_ENEMY(map[row][column])) {
                continue;
            }
            enemy_t enemy;
            initialize_enemy(&enemy, row, column, map_row);
            create_enemy_record(enemy);
        }
    }
}
