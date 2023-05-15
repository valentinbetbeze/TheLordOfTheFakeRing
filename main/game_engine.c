#include "game_engine.h"

#define OUT_OF_RANGE(x)     (3 * LCD_WIDTH / 4 < x)


block_t blocks[NUM_BLOCK_RECORDS];
enemy_t enemies[NUM_ENEMY_RECORDS] = {0};
item_t items[NUM_ITEMS] = {0};
platform_t platforms[MAX_PLATFORMS] = {0};
projectile_t projectiles[MAX_PROJECTILES] = {0};


/*************************************************
 * 'Blocks' function definitions
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


int8_t check_block_collisions(map_t map, physics_t *physics, uint16_t map_row)
{   
    if (map.data == NULL) {
        printf("Error(check_block_collisions): Map file does not exist.\n");
        return -1;
    }
    if (physics == NULL) {
        printf("Error(check_block_collisions): Character cannot be found.\n");
        return -1;
    }
    if (physics->speed_x * 2 >= BLOCK_SIZE || physics->speed_y * 2 >= BLOCK_SIZE) {
        printf("Error(check_block_collisions): Speed is too high. Max speed = (BLOCK_SIZE / 2) - 1\n");
        return -1;
    }

    // Reset collision states
    physics->top_collision    = 0;
    physics->bottom_collision = 0;
    physics->left_collision   = 0;
    physics->right_collision  = 0;

    // Compute reference parameters
    int16_t ref_row = (int16_t)physics->pos_x / BLOCK_SIZE;
    int8_t ref_col, block_tl, block_tr, block_bl, block_br;
    if (physics->pos_y < 0) {
        ref_col = 8;
        block_bl = map.data[ref_row][ref_col - 1];
        block_br = map.data[ref_row + 1][ref_col - 1];
        if (IS_SOLID(block_bl)) block_tl = NON_BREAKABLE_BLOCK_1;
        else block_tl = BACKGROUND_BLOCK;
        if (IS_SOLID(block_br)) block_tr = NON_BREAKABLE_BLOCK_1;
        else block_tr = BACKGROUND_BLOCK;
    }
    else {
        ref_col = NUM_BLOCKS_Y - (int8_t)(physics->pos_y / BLOCK_SIZE) - 1;
        block_tl = map.data[ref_row][ref_col];           // top-left block
        block_tr = map.data[ref_row + 1][ref_col];       // top-right block
        block_bl = map.data[ref_row][ref_col - 1];       // bottom-left block
        block_br = map.data[ref_row + 1][ref_col - 1];   // bottom-right block    
    }

    // Check if the object is within any potential collision area
    struct {
        uint8_t left :      1;
        uint8_t right :     1;
        uint8_t top :       1;
        uint8_t bottom :    1;
    } collision_risk;
    uint8_t x_offset = physics->pos_x % BLOCK_SIZE;
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
    physics->top_collision =        (collision_risk.top && IS_SOLID(block_tl) &&
                                    !is_block_destroyed(ref_row, ref_col) &&
                                    x_offset < BLOCK_SIZE - abs(physics->speed_x) - SLIP_OFFSET) ||
                                    (collision_risk.top && abs(physics->speed_x) + SLIP_OFFSET < x_offset &&
                                    IS_SOLID(block_tr) && !is_block_destroyed(ref_row + 1, ref_col));

    physics->bottom_collision =     (collision_risk.bottom && IS_SOLID(block_bl) &&
                                    !is_block_destroyed(ref_row, ref_col - 1) &&
                                    x_offset < BLOCK_SIZE - abs(physics->speed_x)) ||
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
            set_block_as_hit(ref_row, ref_col, map_row);
        }
    else if (physics->top_collision && IS_INTERACTIVE(block_tr) && !is_block_destroyed(ref_row + 1, ref_col) &&
             BLOCK_SIZE / 2 < x_offset) {
        set_block_as_hit(ref_row + 1, ref_col, map_row);
    }
    return (physics->top_collision || physics->bottom_collision || physics->left_collision || physics->right_collision);
}


void fix_block_collision(physics_t *physics)
{
    uint8_t x_offset = physics->pos_x % BLOCK_SIZE;
    uint8_t y_offset = physics->pos_y % BLOCK_SIZE;
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
 * 'Platforms' function definitions
 *************************************************/

static uint8_t create_platform(map_t map, platform_t *platform)
{
    platform->end_row = platform->start_row;
    platform->end_column = platform->start_column;
    // Determine the trajectory of the platform
    block_type_t block_right = 0;
    block_type_t block_top = 0;
    if (platform->start_row < map.rows - 1) {
        block_right = map.data[platform->start_row + 1][platform->start_column];
    }
    if (platform->start_column < map.columns - 1) {
        block_top = map.data[platform->start_row][platform->start_column + 1];
    }
    if (block_right == PLATFORM_BLOCK && block_top != PLATFORM_BLOCK) {
        // Horizontal platform, keep searching to the right
        platform->horizontal = 1;
        platform->physics.speed_x = SPEED_PLATFORM;
        while (map.data[platform->end_row + 1][platform->start_column] == PLATFORM_BLOCK) {
            platform->end_row++;
            if (map.rows - 1 <= platform->end_row) {
                break;
            }
        }
    }
    else if (block_right != PLATFORM_BLOCK && block_top == PLATFORM_BLOCK) {
        // Vertical platform, keep searching to the top
        platform->vertical = 1;
        platform->physics.speed_y = SPEED_PLATFORM;
        while (map.data[platform->start_row][platform->end_column + 1] == PLATFORM_BLOCK) {
            platform->end_column++;
            if (map.columns - 1 <= platform->end_column) {
                break;
            }
        }
    }
    else if (block_right != PLATFORM_BLOCK && block_top != PLATFORM_BLOCK) {
        static uint8_t create_platform_flag = 0;
        if (!create_platform_flag) {
            printf("Warning(create_platform): Platform at row = %i, column = %i, has no trajectory.\n", platform->start_row, platform->start_column);
        }
        return 0;
    }
    else {
        printf("Error(create_platform): Platform at row = %i, column = %i, has an undetermined trajectory. Please fix the trajectory in map.c, map id: %i\n", platform->start_row, platform->start_column, map.id);
        return 1;
    }
    return 0;
}


platform_t *get_platform(int16_t row, int8_t column)
{
    if (row == -1 || column == -1) {
        return NULL;
    }

    for (int8_t i = 0; i < MAX_PLATFORMS; i++) {
        if (platforms[i].start_row <= row && row <= platforms[i].end_row &&
            platforms[i].start_column <= column && column <= platforms[i].end_column) {
            return &platforms[i];
        }
    }
    return NULL; // Platform not found
}


uint8_t load_platforms(map_t map)
{
    // Initialize array
    for (uint8_t i = 0; i < MAX_PLATFORMS; i++) {
        platforms[i].start_row = -1;
        platforms[i].start_column = -1;
        platforms[i].end_row = -1;
        platforms[i].end_column = -1;
    }
    // Scan the map to find and create platforms
    uint8_t index_platform = 0;
    for (uint16_t row = 0; row < map.rows; row++) {
        for (uint8_t column = 0; column < NUM_BLOCKS_Y; column++) {
            if (map.data[row][column] == PLATFORM_BLOCK) {
                if (MAX_PLATFORMS <= index_platform) {
                    printf("Error(load_platforms): Too many platforms in map: id = %i. Remove some platforms or increase MAX_PLATFORMS in project_config.h\n", map.id);
                    return 1;
                }
                else if (get_platform(row, column)) {
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
                if (create_platform(map, &platform)) {
                    return 1;
                }
                platforms[index_platform] = platform;
                index_platform++;
            }
        }
    }
    return 0;
}


int8_t check_platform_collision(physics_t *physics)
{
    if (physics == NULL) {
        printf("Error(check_block_collisions): `physics_t` entity cannot be found.\n");
        return -1;
    }
    for (uint8_t i = 0; i < MAX_PLATFORMS; i++) {
        // Abort if no more platforms to check
        if (platforms[i].start_row == -1 || physics->pos_x < (platforms[i].start_row - 1) * BLOCK_SIZE) {
            break;
        }
        // Check collision
        uint8_t on_platform_x, on_hplatform_y, on_vplatform_y;
        on_platform_x = (platforms[i].physics.pos_x - BLOCK_SIZE + 2 < physics->pos_x) &&
                        (physics->pos_x < platforms[i].physics.pos_x + 2 * BLOCK_SIZE - 2);

        on_hplatform_y =    (platforms[i].physics.pos_y - BLOCK_SIZE < physics->pos_y) &&
                            (physics->pos_y <= platforms[i].physics.pos_y - BLOCK_SIZE + physics->speed_y);
        
        on_vplatform_y = (
            (platforms[i].moved && platforms[i].physics.pos_y - BLOCK_SIZE <= physics->pos_y &&
            physics->pos_y <= platforms[i].physics.pos_y - BLOCK_SIZE - platforms[i].physics.speed_y + physics->speed_y)
            ||  (!platforms[i].moved && platforms[i].physics.pos_y - BLOCK_SIZE < physics->pos_y &&
                physics->pos_y <= platforms[i].physics.pos_y - BLOCK_SIZE + physics->speed_y));
 
        if ((platforms[i].horizontal && on_platform_x && on_hplatform_y) ||
            (platforms[i].vertical && on_platform_x && on_vplatform_y)) {
            physics->bottom_collision = 1;
            physics->platform_i = i;
            if (physics->grounded) {
                physics->left_collision |= (physics->pos_x < platforms[i].physics.pos_x);
                physics->right_collision |= (platforms[i].physics.pos_x + BLOCK_SIZE < physics->pos_x);
            }
            return 1;
        }
    }
    physics->platform_i = -1;
    return 0;
}


/*************************************************
 * 'Enemies / Player' function definitions
 *************************************************/

void set_stationary_enemy_orientation(map_t map, enemy_t *enemy)
{
    if (!enemy->stationary) {
        return;
    }
    int8_t left_block = map.data[enemy->row - 1][enemy->column];
    int8_t right_block = map.data[enemy->row + 1][enemy->column];
    if (IS_SOLID(left_block) && !IS_SOLID(right_block)) {
        enemy->physics.speed_x = 1;
    }
    else if (!IS_SOLID(left_block) && IS_SOLID(right_block)) {
        enemy->physics.speed_x = -1;
    }
    else {
        // Default orientation
        printf("Warning(set_stationary_enemy_orientation): Cannot determine orientation for stationary enemy at row: %i, column: %i. Default orientation applied.\n", enemy->row, enemy->column);
        enemy->physics.speed_x = -1;
    }
}


uint8_t create_enemy_record(enemy_t enemy, uint16_t map_x)
{
    // Scan the enemy log to create the state record of an enemy
    for (int8_t i = 0; i < NUM_ENEMY_RECORDS; i++) {
        if (enemies[i].row == enemy.row && enemies[i].column == enemy.column) {
            printf("Error(create_enemy_record): Record cannot be created, enemy at row: %i, column: %i, already exists.\n", enemies[i].row, enemies[i].column);
            return 1;
        }
        else if (!enemies[i].row || enemies[i].physics.pos_x < map_x - LCD_WIDTH) {
            /* If the slot is free (= 0) or the enemy is beyond the previous frame,
            clean the slot and write over it */
            memset(&enemies[i], 0, sizeof(enemies[i]));
            enemies[i] = enemy;
            return 0;
        }
    }
    printf("Error(create_enemy_record): Record cannot be created, enemies[] array is full.\n");
    return 1;
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


uint8_t spawn_enemies(map_t map, uint16_t map_x, int16_t start_row, int16_t end_row)
{
    for (uint8_t row = start_row; row < end_row; row++) {
        for (int column = 0; column < NUM_BLOCKS_Y; column++) {
            if (!IS_ENEMY(map.data[row][column])) {
                continue;
            }
            if (get_enemy_record(row, column)) {
                continue; // Enemy has already been spawned
            }
            enemy_t enemy = {
                .row                = row,
                .column             = column,
                .life               = 1,
                .physics.platform_i = -1,
                .physics.speed_x    = -1 * SPEED_INITIAL,
                .physics.speed_y    = SPEED_INITIAL
            };
            if (map.data[row][column] == ENEMY_2) {
                enemy.physics.grounded = 1;
            }
            else if (map.data[row][column] == ENEMY_3) {
                enemy.stationary =1;
                set_stationary_enemy_orientation(map, &enemy);
            }
            // Check if the enemy is to be spawned on a platform
            platform_t *platform = get_platform(row, column - 1);
            if (platform == NULL) {
                enemy.physics.pos_x = BLOCK_SIZE * row;
                enemy.physics.pos_y = BLOCK_SIZE * (NUM_BLOCKS_Y - 1 - column);
            }
            else {
                enemy.physics.pos_x = platform->physics.pos_x + BLOCK_SIZE;
                enemy.physics.pos_y = platform->physics.pos_y - BLOCK_SIZE;
            }
            // Create the enemy record
            if (create_enemy_record(enemy, map_x)) {
                return 1;
            }
        }
    }
    return 0;
}


uint8_t initiate_jump(physics_t *physics, uint8_t initial_speed)
{
    if (physics == NULL) {
        printf("Error(initiate_jump): `physics_t` entity cannot be found.\n");
        return 1;
    }
    if (initial_speed * 2 >= BLOCK_SIZE) {
        printf("Error(initiate_jump): Initial speed is too high. Max speed = (BLOCK_SIZE / 2) - 1\n");
        return 1;
    }
    physics->jumping = 1;
    physics->falling = 0;
    physics->speed_y = initial_speed;
    return 0;
}


int8_t is_on_sight(map_t map, physics_t *shooter, physics_t *target)
{
    if (shooter == NULL) {
        printf("Error(is_on_sight): `physics_t` shooter cannot be found.\n");
        return -1;
    }
    if (target == NULL) {
        printf("Error(is_on_sight): `physics_t` target cannot be found.\n");
        return -1;
    }
    // Check if on range
    int16_t dist_x = target->pos_x - shooter->pos_x;
    int16_t dist_y = target->pos_y - shooter->pos_y;
    uint16_t dist = hypot(abs(dist_x), abs(dist_y));
    if (OUT_OF_RANGE(dist)) {
        return 0;
    }

    int16_t h_adja_block_row = shooter->pos_x / BLOCK_SIZE;
    int16_t h_adja_block_col = NUM_BLOCKS_Y - (shooter->pos_y / BLOCK_SIZE) - 1;
    int16_t v_adja_block_row = h_adja_block_row;
    int16_t v_adja_block_col = h_adja_block_col;
    // Scan the path between the shooter and the target
    while (BLOCK_SIZE < dist) {       
        if (0 < dist_x) h_adja_block_row++;
        else if (dist_x < 0) h_adja_block_row--;
        if (0 < dist_y) v_adja_block_col--;
        else if (dist_y < 0) v_adja_block_col++; 
        // Horizontal adjacent block
        dist_x = target->pos_x - BLOCK_SIZE * h_adja_block_row;
        dist_y = target->pos_y - BLOCK_SIZE * (NUM_BLOCKS_Y - h_adja_block_col - 1);
        uint16_t h_adja_block_dist = hypot(abs(dist_x), abs(dist_y));
        // Vertical adjacent block
        dist_x = target->pos_x - BLOCK_SIZE * v_adja_block_row;
        dist_y = target->pos_y - BLOCK_SIZE * (NUM_BLOCKS_Y - v_adja_block_col - 1);
        uint16_t v_adja_block_dist = hypot(abs(dist_x), abs(dist_y));
        // Check for solid block on the shortest path
        if (h_adja_block_dist < v_adja_block_dist) {
            if (IS_SOLID(map.data[h_adja_block_row][h_adja_block_col])) {
                return 0; // Obstacle found
            }
            // If not solid, carry on the analysis
            v_adja_block_row = h_adja_block_row;
            v_adja_block_col = h_adja_block_col;
            dist = h_adja_block_dist;
        }
        else if (v_adja_block_dist < h_adja_block_dist) {
            if (IS_SOLID(map.data[v_adja_block_row][v_adja_block_col])) {
                return 0; // Obstacle found
            }
            // If not solid, carry on the analysis
            h_adja_block_row = v_adja_block_row;
            h_adja_block_col = v_adja_block_col;
            dist = v_adja_block_dist;
        }
        else {
            if (IS_SOLID(map.data[h_adja_block_row][v_adja_block_col])) {
                return 0; // Obstacle found
            }
            // If not solid, carry on the analysis
            dist_x = target->pos_x - BLOCK_SIZE * h_adja_block_row;
            dist_y = target->pos_y - BLOCK_SIZE * (NUM_BLOCKS_Y - v_adja_block_col - 1);
            h_adja_block_col = v_adja_block_col;
            v_adja_block_row = h_adja_block_row;
            dist = hypot(abs(dist_x), abs(dist_y));
        }
    }
    // If no block is solid, the target is on sight
    return 1;
}


uint8_t initiate_shoot(physics_t *shooter, physics_t *target)
{
    if (shooter == NULL) {
        printf("Error(initiate_shoot): `physics_t` shooter cannot be found.\n");
        return 1;
    }
    if (target == NULL) {
        printf("Error(initiate_shoot): `physics_t` target cannot be found.\n");
        return 1;
    }
    // Create the projectile
    int16_t dist_x = target->pos_x - shooter->pos_x;
    int16_t dist_y = target->pos_y - shooter->pos_y;
    projectile_t projectile = {0};
    if (0 < dist_x) {
        // The target is at the right of the shooter
        projectile.physics.pos_x = shooter->pos_x + BLOCK_SIZE;
        projectile.physics.speed_x = 1;
    }
    else {
        // The target is at the left of the shooter
        projectile.physics.pos_x = shooter->pos_x - BLOCK_SIZE;
        projectile.physics.speed_x = -1;
    }
    projectile.physics.pos_y = shooter->pos_y;
    projectile.slope = (float)dist_y / dist_x;
    projectile.offset = shooter->pos_y - projectile.slope * shooter->pos_x;
    // Store the projectile in memory
    for (uint8_t i = 0; i < MAX_PROJECTILES; i++) {
        if (projectiles[i].physics.speed_x == 0) {
            projectiles[i] = projectile;
            return 0;
        }
        else {
            uint16_t dist_x = abs(projectiles[i].physics.pos_x - target->pos_x);
            uint16_t dist_y = abs(projectiles[i].physics.pos_y - target->pos_y);
            int16_t dist = hypot(dist_x, dist_y);
            if (OUT_OF_RANGE(dist)) {
                memset(&projectiles[i], 0, sizeof(projectiles[i]));
                projectiles[i] = projectile;
                return 0;
            }
        }
    }
    printf("Error(initiate_shoot): Could not create projectile (projectiles[] is full).\n");
    return 1;
}


/*************************************************
 * 'Items' function definitions
 *************************************************/

item_t generate_item(int16_t row, int8_t column, uint16_t map_x)
{
    uint8_t random = esp_random() % 100;
    item_t item = {
        .spawned = 1,
        .sprite.height = BLOCK_SIZE,
        .sprite.width = BLOCK_SIZE,
        .sprite.pos_x = BLOCK_SIZE * row - map_x,
        .sprite.pos_y = LCD_HEIGHT - (BLOCK_SIZE * (column + 1)) - 1,
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

