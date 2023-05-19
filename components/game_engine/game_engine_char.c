#include "game_engine.h"

#define OUT_OF_RANGE(x)     (3 * LCD_WIDTH / 4 < x)

enemy_t enemies[NUM_ENEMY_RECORDS] = {0};
projectile_t projectiles[MAX_PROJECTILES] = {0};


/**
 * @brief Set the orientation of a stationary enemy.
 * 
 * @param map Pointer to the current game map.
 * @param enemy Pointer of the enemy which position must be set.
 * 
 * @note The function checks the adjacent blocks of the stationary enemy
 * to determine if it is facing any wall. If so, the enemy will face the
 * opposite direction.
 */
static void set_stationary_enemy_orientation(const map_t *map, enemy_t *enemy)
{
    if (map == NULL) {
        printf("Error(set_stationary_enemy_orientation): map_t pointer is NULL.\n");
        assert(map);
    }
    if (map->data == NULL) {
        printf("Error(set_stationary_enemy_orientation): Map data pointer is NULL.\n");
        assert(map->data);
    }
    if (enemy == NULL) {
        printf("Error(set_stationary_enemy_orientation): enemy_t pointer is NULL.\n");
        assert(enemy);
    }
    if (!enemy->stationary) {
        return;
    }
    const int8_t left_block = map->data[enemy->row - 1][enemy->column];
    const int8_t right_block = map->data[enemy->row + 1][enemy->column];
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


/**
 * @brief Configure the in-game properties of an enemy based on its type.
 * 
 * @param map Pointer to the current game map.
 * @param enemy Pointer of the enemy to configure.
 * 
 * @note The type of the enemy is determined through the row and column
 * members of the enemy_t object pointer. Hence, the enemy_t object must be
 * pre-set for the configuration to work correctly.
 */
static void configure_enemy(const map_t *map, enemy_t *enemy)
{
    if (map == NULL) {
        printf("Error(configure_enemy): map_t pointer is NULL.\n");
        assert(map);
    }
    if (map->data == NULL) {
        printf("Error(configure_enemy): Map data pointer is NULL.\n");
        assert(map->data);
    }
    if (enemy == NULL) {
        printf("Error(configure_enemy): enemy_t pointer is NULL.\n");
        assert(enemy);
    }
    if (enemy->row == -1 || enemy->column == -1) {
        printf("Error(configure_enemy): enemy_t object has not been set correcly. Please make sure the base row and column of the enemy are set first.\n");
        assert(enemy->row != -1 && enemy->column != -1);
    }
    enemy->life = 1;
    enemy->physics.platform_i = -1;
    enemy->physics.speed_x = -1 * SPEED_INITIAL;
    enemy->physics.speed_y = SPEED_INITIAL;
    switch (map->data[enemy->row][enemy->column]) {
        case ENEMY_2:
            enemy->physics.grounded = 1;
            break;
        case ENEMY_3:
            enemy->stationary = 1;
            set_stationary_enemy_orientation(map, enemy);
            break;
        case ENEMY_4:
            enemy->infinite_spawn = 1;
            break;
        default:
            break;
    }
    // Check if the enemy is to be spawned on a platform
    uint8_t platform_index;
    if (get_platform(&platform_index, enemy->row, enemy->column - 1)) {
        enemy->physics.pos_x = platforms[platform_index].physics.pos_x + BLOCK_SIZE;
        enemy->physics.pos_y = platforms[platform_index].physics.pos_y - BLOCK_SIZE;
    }
    else {
        enemy->physics.pos_x = BLOCK_SIZE * enemy->row;
        enemy->physics.pos_y = BLOCK_SIZE * (NUM_BLOCKS_Y - 1 - enemy->column);
    }
}


/**
 * @brief Store an enemy in memory.
 * 
 * @param enemy Pointer of the enemy to store in memory.
 * @param cam_pos_x x-position of the camera in pixels.
 * 
 * @return 1 if the enemy has been stored successfully, else 0.
 */
static uint8_t store_enemy(const enemy_t *enemy, const uint16_t cam_pos_x)
{
    if (enemy == NULL) {
        printf("Error(store_enemy): enemy_t pointer is NULL.\n");
        assert(enemy);
    }
    // Scan the enemy log to create the state record of an enemy
    for (int8_t i = 0; i < NUM_ENEMY_RECORDS; i++) {
        if (enemies[i].row == enemy->row && enemies[i].column == enemy->column) {
            return 0; // Record already exists.
        }
        else if (!enemies[i].row || enemies[i].physics.pos_x < cam_pos_x - LCD_WIDTH) {
            /* If the slot is free (= 0) or the enemy is beyond the previous frame,
            clean the slot and write over it */
            memset(&enemies[i], 0, sizeof(enemies[i]));
            enemies[i] = *enemy;
            return 1;
        }
    }
    printf("Error(store_enemy): Record cannot be created, enemies[] array is full.\n");
    assert(0);
}


/**
 * @brief Check if an enemy has been spawned (e.i. still in memory).
 * 
 * @param row Initial row of the enemy.
 * @param column Initial column of the enemy.
 * 
 * @return 1 if the enemy has been spawned, else 0.
 * 
 * @warning Depending on their properties, some enemies are erased from memory
 * at their death, allowing for a continuous respawn, while others are not.
 */
static uint8_t is_enemy_spawned(const int16_t row, const int8_t column)
{
    for (int8_t i = 0; i < NUM_ENEMY_RECORDS; i++) {
        if (enemies[i].row == row && enemies[i].column == column) {
            return 1;
        }
    }
    return 0; // Enemy not found
}


void spawn_enemies(const map_t *map, const uint16_t cam_pos_x, const int16_t start_row, const int16_t end_row)
{
    if (map == NULL) {
        printf("Error(spawn_enemies): map_t pointer is NULL.\n");
        assert(map);
    }
    if (map->data == NULL) {
        printf("Error(spawn_enemies): Map data pointer is NULL.\n");
        assert(map->data);
    }
    for (uint8_t row = start_row; row < end_row; row++) {
        if (map->end_row < row) {
            break;
        }
        for (uint8_t column = 0; column < NUM_BLOCKS_Y; column++) {
            if (!IS_ENEMY(map->data[row][column])) {
                continue;
            }
            if (is_enemy_spawned(row, column)) {
                continue;
            }
            // Create an enemy, configure it and store it in memory
            enemy_t enemy = {
                .row = row,
                .column = column,
            };
            configure_enemy(map, &enemy);
            if (!store_enemy(&enemy, cam_pos_x)) {
                printf("Error(spawn_enemies): Enemy record creation failed for enemy at row: %i, column: %i\n", enemy.row, enemy.column);
                assert(0);
            }
        }
    }
}


void initiate_jump(physics_t *physics, const uint8_t initial_speed)
{
    if (physics == NULL) {
        printf("Error(initiate_jump): physics_t pointer is NULL.\n");
        assert(physics);
    }
    if (initial_speed * 2 >= BLOCK_SIZE) {
        printf("Error(initiate_jump): Initial speed is too high. Max speed = (BLOCK_SIZE / 2) - 1\n");
        assert(initial_speed * 2 < BLOCK_SIZE);
    }
    physics->jumping = 1;
    physics->falling = 0;
    physics->speed_y = initial_speed;
}


uint8_t is_on_sight(const map_t *map, physics_t *shooter, physics_t *target)
{
    if (map == NULL) {
        printf("Error(is_on_sight): map_t pointer is NULL.\n");
        assert(map);
    }
    if (map->data == NULL) {
        printf("Error(is_on_sight): Map data pointer is NULL.\n");
        assert(map->data);
    }
    if (shooter == NULL) {
        printf("Error(is_on_sight): physics_t `shooter` pointer is NULL.\n");
        assert(shooter);
    }
    if (target == NULL) {
        printf("Error(is_on_sight): physics_t `target` pointer is NULL.\n");
        assert(target);
    }
    // Check if on range
    int16_t dist_x = target->pos_x - shooter->pos_x;
    int16_t dist_y = target->pos_y - shooter->pos_y;
    uint16_t dist = hypot(abs(dist_x), abs(dist_y));
    if (OUT_OF_RANGE(dist)) {
        return 0;
    }
    // If on range, proceed to check if on sight
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
            if (IS_SOLID(map->data[h_adja_block_row][h_adja_block_col])) {
                return 0; // Obstacle found
            }
            // If not solid, carry on the analysis
            v_adja_block_row = h_adja_block_row;
            v_adja_block_col = h_adja_block_col;
            dist = h_adja_block_dist;
        }
        else if (v_adja_block_dist < h_adja_block_dist) {
            if (IS_SOLID(map->data[v_adja_block_row][v_adja_block_col])) {
                return 0; // Obstacle found
            }
            // If not solid, carry on the analysis
            h_adja_block_row = v_adja_block_row;
            h_adja_block_col = v_adja_block_col;
            dist = v_adja_block_dist;
        }
        else {
            if (IS_SOLID(map->data[h_adja_block_row][v_adja_block_col])) {
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


void shoot_projectile(const physics_t *shooter, const physics_t *target)
{
    if (shooter == NULL) {
        printf("Error(shoot_projectile): physics_t `shooter` pointer is NULL.\n");
        assert(shooter);
    }
    if (target == NULL) {
        printf("Error(shoot_projectile): physics_t `target` pointer is NULL.\n");
        assert(target);
    }
    // Create the projectile
    projectile_t projectile = {0};
    if (0 < target->pos_x - shooter->pos_x) {
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
    projectile.slope = (float)(target->pos_y - shooter->pos_y) / (target->pos_x - shooter->pos_x);
    projectile.offset = shooter->pos_y - projectile.slope * shooter->pos_x;
    projectile.physics.platform_i = -1;
    // Store the projectile in memory
    for (uint8_t i = 0; i < MAX_PROJECTILES; i++) {
        if (projectiles[i].physics.speed_x == 0) {
            projectiles[i] = projectile;
            return;
        }
        else {
            const uint16_t dist_x = abs(projectiles[i].physics.pos_x - target->pos_x);
            const uint16_t dist_y = abs(projectiles[i].physics.pos_y - target->pos_y);
            if (OUT_OF_RANGE(hypot((uint16_t)dist_x, (uint16_t)dist_y))) {
                memset(&projectiles[i], 0, sizeof(projectiles[i]));
                projectiles[i] = projectile;
                return;
            }
        }
    }
    printf("Error(shoot_projectile): Could not create projectile (projectiles[] is full).\n");
    assert(0);
}
