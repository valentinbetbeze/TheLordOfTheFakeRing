#include "game_engine.h"

block_t blocks[NUM_BLOCK_RECORDS];
enemy_t enemies[NUM_ENEMY_RECORDS] = {0};
item_t items[NUM_ITEMS] = {0};


/*************************************************
 * Blocks
 *************************************************/

static uint8_t is_block_destroyed(int16_t row, int8_t column)
{
    block_t *block = (block_t *)get_block_record(row, column);
    if (block != NULL && block->destroyed) {
        return 1;
    }
    return 0;
}


static uint8_t set_block_as_hit(int16_t row, int8_t column, uint16_t map_row)
{
    block_t new_block = {
        .row = row,
        .column = column,
        .is_hit = 1
    };
    uint8_t err = create_block_record(new_block, map_row);
    if (err == 1) {
        block_t *existing_block = get_block_record(row, column);
        existing_block->is_hit = 1;
    }
    else if (err == 2) {
        return 1;
    }
    return 0;
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


uint8_t create_block_record(block_t block, uint16_t map_row)
{
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
        else if (!free_slot_found && blocks[i].row < map_row) {
            blocks[i].row          = -1; // Empty slot identifier
            blocks[i].column       = -1;
            blocks[i].is_hit       = 0;
            blocks[i].destroyed    = 0;
            blocks[i].item_given   = 0;
            blocks[i].bumping      = 0;
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

    // Compute reference parameters
    int16_t ref_row = (int16_t)(physics->pos_x + map_x) / BLOCK_SIZE + 1;
    int8_t ref_col, block_tl, block_tr, block_bl, block_br;
    if (physics->pos_y < 0) {
        ref_col = 8;
        block_bl = map[ref_row][ref_col - 1];
        block_br = map[ref_row + 1][ref_col - 1];
        if (block_bl) block_tl = NON_BREAKABLE_BLOCK_1;
        else block_tl = BACKGROUND_BLOCK;
        if (block_br) block_tr = NON_BREAKABLE_BLOCK_1;
        else block_tr = BACKGROUND_BLOCK;
    }
    else {
        ref_col = NUM_BLOCKS_Y - (int8_t)(physics->pos_y / BLOCK_SIZE) - 1;
        block_tl = map[ref_row][ref_col];           // top-left block
        block_tr = map[ref_row + 1][ref_col];       // top-right block
        block_bl = map[ref_row][ref_col - 1];       // bottom-left block
        block_br = map[ref_row + 1][ref_col - 1];   // bottom-right block    
    }

    // Check if the object is within any potential collision area
    struct {
        uint8_t left :      1;
        uint8_t right :     1;
        uint8_t top :       1;
        uint8_t bottom :    1;
    } collision_risk;
    uint8_t x_offset = (physics->pos_x + map_x) % BLOCK_SIZE;
    uint8_t y_offset = physics->pos_y % BLOCK_SIZE;
    collision_risk.top = (BLOCK_SIZE / 2 < y_offset) && (y_offset <= BLOCK_SIZE - 1);
    collision_risk.bottom = (1 <= y_offset) && (y_offset < (uint8_t)BLOCK_SIZE / 2) &&
                            (physics->pos_y < LCD_HEIGHT - BLOCK_SIZE);
    if ((physics->jumping && IS_SOLID(block_tl)) || (physics->falling && IS_SOLID(block_br))) {
        collision_risk.right = (1 <= x_offset) && (x_offset <= abs(physics->speed_x));
    }
    else {
        collision_risk.right = (1 <= x_offset) && (x_offset <= abs(physics->speed_x) + SLIP_OFFSET);
    }
    if ((physics->jumping && IS_SOLID(block_tr)) || (physics->falling && IS_SOLID(block_bl))) {
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
    physics->top_collision =    (collision_risk.top && IS_SOLID(block_tl) &&
                                !is_block_destroyed(ref_row, ref_col) &&
                                x_offset < BLOCK_SIZE - abs(physics->speed_x) - SLIP_OFFSET) ||
                                (collision_risk.top && abs(physics->speed_x) + SLIP_OFFSET < x_offset &&
                                IS_SOLID(block_tr) && !is_block_destroyed(ref_row + 1, ref_col));

    physics->bottom_collision = (collision_risk.bottom && IS_SOLID(block_bl) &&
                                !is_block_destroyed(ref_row, ref_col - 1) &&
                                x_offset < BLOCK_SIZE - abs(physics->speed_x)) ||
                                (collision_risk.bottom && abs(physics->speed_x) < x_offset &&
                                IS_SOLID(block_br) && !is_block_destroyed(ref_row + 1, ref_col - 1));

    physics->left_collision =   (collision_risk.left && IS_SOLID(block_tl) && !is_block_destroyed(ref_row, ref_col)) ||
                                (physics->grounded && x_offset &&
                                (!IS_SOLID(block_bl) || is_block_destroyed(ref_row, ref_col - 1)));

    physics->right_collision =  (collision_risk.right && IS_SOLID(block_tr) && !is_block_destroyed(ref_row + 1, ref_col)) ||
                                (physics->grounded && x_offset && 
                                (!IS_SOLID(block_br) || is_block_destroyed(ref_row + 1, ref_col - 1)));
                                
    if (physics->jumping || physics->falling) {
        physics->left_collision |=  (collision_risk.left && y_offset && IS_SOLID(block_bl) && 
                                    !is_block_destroyed(ref_row, ref_col - 1) &&
                                    (!IS_SOLID(block_br) || is_block_destroyed(ref_row + 1, ref_col - 1)));

        physics->right_collision |= (collision_risk.right && y_offset && IS_SOLID(block_br) && 
                                    !is_block_destroyed(ref_row + 1, ref_col - 1) &&
                                    (!IS_SOLID(block_bl) || is_block_destroyed(ref_row, ref_col - 1)));
    }
    // Change interactive blocks state if appropriate
    if (physics->top_collision && IS_INTERACTIVE(block_tl) && !is_block_destroyed(ref_row, ref_col) &&
        x_offset <= BLOCK_SIZE / 2) {
            set_block_as_hit(ref_row, ref_col, (uint16_t)(map_x / BLOCK_SIZE) + 1);
        }
    else if (physics->top_collision && IS_INTERACTIVE(block_tr) && !is_block_destroyed(ref_row + 1, ref_col) &&
             BLOCK_SIZE / 2 < x_offset) {
        set_block_as_hit(ref_row + 1, ref_col, (uint16_t)(map_x / BLOCK_SIZE) + 1);
    }
    return (physics->top_collision || physics->bottom_collision || physics->left_collision || physics->right_collision);
}


void fix_position(physics_t *physics, uint16_t map_x)
{
    uint8_t x_offset = (physics->pos_x + map_x) % BLOCK_SIZE;
    uint8_t y_offset = physics->pos_y % BLOCK_SIZE;
    // x-direction
    if (physics->left_collision) {
        physics->pos_x += BLOCK_SIZE - x_offset;
    }
    else if (physics->right_collision) {
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


void bump_block(block_t *block, sprite_t *sprite, uint64_t timer)
{
    static uint64_t t0 = 0;
    static uint8_t steps = 0;

    if ((timer - t0) / TIMESTEP_BUMP_BLOCK < 1) {
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


/*************************************************
 * Enemies
 *************************************************/

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
    enemy->physics.speed_x          = -1 * SPEED_INITIAL;
    enemy->physics.speed_y          = SPEED_INITIAL;
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
        printf("Error(create_enemy_record): Cannot add new enemy (enemies[] full).\n");
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
            enemy_t enemy;
            if (!IS_ENEMY(map[row][column])) {
                continue;
            }
            else if (map[row][column] == ENEMY_2) {
                enemy.physics.grounded = 1;
            }
            initialize_enemy(&enemy, row, column, map_row);
            create_enemy_record(enemy);
        }
    }
}


/*************************************************
 * Items
 *************************************************/

item_t generate_item(int16_t row, int8_t column, uint16_t map_x)
{
    uint8_t random = esp_random() % 100;
    item_t item = {
        .spawned = 1,
        .sprite.height = BLOCK_SIZE,
        .sprite.width = BLOCK_SIZE,
        .sprite.pos_x = BLOCK_SIZE * (row - 1) - map_x,
        .sprite.pos_y = LCD_HEIGHT - (BLOCK_SIZE * (column + 1)),
    };
    if (random < 3) {
        item.type = LIGHTSTAFF;
        item.sprite.data = sprite_lightstaff;
    }
    else if (random < 6) {
        item.type = SHIELD;
        item.sprite.data = sprite_shield;
    }
    else {
        item.type = COIN;
        item.sprite.data = sprite_coin_1;
    }
    return item;
}


uint8_t store_item(item_t item)
{
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
        return 1;
    }
    memset(&items[index], 0, sizeof(items[index]));
    items[index] = item;
    return 0; 
}

