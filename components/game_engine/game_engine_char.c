#include "game_engine.h"

#define OUT_OF_RANGE(x)     (3 * LCD_WIDTH / 4 < x)


enemy_t enemies[NUM_ENEMY_RECORDS] = {0};
projectile_t projectiles[MAX_PROJECTILES] = {0};


/*************************************************
 * Projectile functions
 *************************************************/

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


void compute_projectile(game_t *game, player_t *player, projectile_t *projectile)
{
    if (game == NULL) {
        printf("Error(update_projectile): game_t pointer is NULL.\n");
        assert(game);
    }
    if (player == NULL) {
        printf("Error(update_projectile): player_t pointer is NULL.\n");
        assert(player);
    }
    if (projectile == NULL) {
        printf("Error(update_projectile): projectile_t pointer is NULL.\n");
        assert(projectile);
    }
    if (projectile->physics.speed_x == 0) {
        return;
    }
    // Update position
    projectile->physics.pos_x += projectile->physics.speed_x;
    projectile->physics.pos_y = projectile->slope * projectile->physics.pos_x + projectile->offset;
    // Check for collision with the environment
    if (check_block_collisions(game->map, &projectile->physics, NULL, game->cam_row)) {
        memset(projectile, 0, sizeof(*projectile));
    }
    // Check for collision with the player
    if (player->physics.pos_x < projectile->physics.pos_x + BLOCK_SIZE / 2 + HITBOX_PROJECTILE / 2 &&
        player->physics.pos_x > projectile->physics.pos_x - BLOCK_SIZE / 2 - HITBOX_PROJECTILE / 2 &&
        player->physics.pos_y < projectile->physics.pos_y + BLOCK_SIZE / 2 + HITBOX_PROJECTILE / 2 &&
        player->physics.pos_y > projectile->physics.pos_y - BLOCK_SIZE / 2 - HITBOX_PROJECTILE / 2) {
        memset(projectile, 0, sizeof(*projectile));
        if (player->shield) {
            player->shield = 0;
        }
        else {
            player->life--;
            game->reset = 1;
        }
    }
}


/*************************************************
 * Player functions
 *************************************************/

void check_player_state(game_t *game, player_t *player, const uint8_t is_jumping)
{
    if (game == NULL) {
        printf("Error(check_player_states): game_t pointer is NULL.\n");
        assert(game);
    }
    if (player == NULL) {
        printf("Error(check_player_states): player_t pointer is NULL.\n");
        assert(player);
    }
    // Check if the player fell
    if (LCD_HEIGHT + BLOCK_SIZE <= player->physics.pos_y) {
        player->life--;
        game->reset = 1;
        return;
    }
    if (player->physics.falling && player->physics.bottom_collision) {
        player->physics.falling = 0;
        player->physics.speed_y = SPEED_INITIAL;
    }
    else if ((player->physics.bottom_collision || player->physics.left_collision ||
                player->physics.right_collision) && is_jumping) {
        player->timer = (uint32_t)game->timer;
        initiate_jump(&player->physics, SPEED_JUMP_INIT);
    }
    else if ((player->physics.jumping && player->physics.top_collision) ||
            (player->physics.jumping && player->physics.speed_y == 0)) {
        player->physics.jumping = 0;
        player->physics.falling = 1;
    }
    else if (!player->physics.jumping && !player->physics.bottom_collision) {
        player->physics.falling = 1;
    }
}


void update_player_position(game_t *game, player_t *player, const int8_t x_axis_value)
{
    if (game == NULL) {
        printf("Error(update_player_x): game_t pointer is NULL.\n");
        assert(game);
    }
    if (player == NULL) {
        printf("Error(update_player_x): player_t pointer is NULL.\n");
        assert(player);
    }
    // Update x-position
    if (0 < x_axis_value / 100) {
        player->forward = 1;
    }
    else if (x_axis_value / 100 < 0){
        player->forward = 0;
    }
    player->physics.pos_x += player->physics.speed_x * (int16_t)(x_axis_value / 100);
    if (player->physics.pos_x - game->cam_pos_x + BLOCK_SIZE / 2 > LCD_WIDTH / 2) {
        game->cam_moving = 1;
        game->cam_pos_x += player->physics.speed_x; // Shift the map frame by 'speed_x' pixels
        game->cam_row = (uint16_t)(game->cam_pos_x / BLOCK_SIZE);
    }
    else if (player->physics.pos_x < game->cam_pos_x) {
        player->physics.pos_x = game->cam_pos_x;
    }
    else {
        game->cam_moving = 0;
    }
    // Update y-position
    player->physics.accelerating = (game->timer - player->timer) / TIMESTEP_ACCEL;
    if (player->physics.jumping) {
        if (player->physics.accelerating) {
            player->timer = (uint32_t)game->timer;
            player->physics.accelerating = 0;
            player->physics.speed_y--; // Decelerating (due to gravity)
        }
        player->physics.pos_y -= player->physics.speed_y;
    }
    else {
        if (player->physics.falling && player->physics.accelerating &&
            player->physics.speed_y < BLOCK_SIZE / 2 - 1) {
            player->timer = (uint32_t)game->timer;
            player->physics.accelerating = 0;
            player->physics.speed_y++;
        }
        player->physics.pos_y += player->physics.speed_y; // Gravity
    }
}


/*************************************************
 * Enemy functions
 *************************************************/

/**
 * @brief Set the orientation of a stationary enemy.
 * 
 * @param map Pointer to the current game map.
 * @param enemy Pointer of the enemy to orientate.
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


/**
 * @brief Update the state of an enemy.
 * 
 * @param enemy Pointer to the enemy_t object to update.
 * 
 * @return 1 if the enemy is dead, else 0
 */
static uint8_t update_enemy_state(enemy_t *enemy)
{
    if ((enemy->life == 0 && !enemy->physics.grounded) ||
        (LCD_HEIGHT < enemy->physics.pos_y && !enemy->infinite_spawn)) {
        memset(enemy, 0, sizeof(*enemy)); // Remove from memory
        return 1;
    }
    else if (enemy->life == 0) {
        return 1;
    }
    // Check if falling
    if (!enemy->physics.bottom_collision) {
        enemy->physics.falling = 1;
    }
    else if (enemy->physics.falling && enemy->physics.bottom_collision) {
        enemy->physics.falling = 0;
        enemy->physics.speed_y = SPEED_INITIAL;
    }
    return 0;
}


/**
 * @brief Update the position of a given enemy.
 * 
 * @param game Pointer to the current game map.
 * @param enemy Pointer to the enemy to update.
 */
static void update_enemy_position(game_t *game, enemy_t *enemy)
{
    // Update x-direction
    const uint8_t turn_around = (!enemy->physics.falling && 
                                (enemy->physics.left_collision || enemy->physics.right_collision));
    if (turn_around) {
        enemy->physics.speed_x *= -1;
    }
    // Update x-position
    if (!enemy->stationary && !(enemy->infinite_spawn && enemy->physics.falling) && 
        (((game->timer - enemy->timer_x) / TIMESTEP_ENEMY > 1) || turn_around)) {

        enemy->timer_x = (uint32_t)game->timer;
        enemy->physics.pos_x += enemy->physics.speed_x;
    }
    // Update y-position
    enemy->physics.accelerating = (game->timer - enemy->timer_y) / TIMESTEP_ACCEL;
    if (enemy->infinite_spawn && (5 * LCD_HEIGHT < enemy->physics.pos_y)) {
        enemy->physics.pos_x = enemy->row * BLOCK_SIZE;
        enemy->physics.pos_y = -1 * BLOCK_SIZE;
        enemy->physics.speed_y = SPEED_INITIAL;
        enemy->physics.falling = 1;
    }
    else if (enemy->physics.falling && enemy->physics.accelerating &&
            enemy->physics.speed_y < BLOCK_SIZE / 2 - 1) {
        enemy->physics.accelerating = 0;
        enemy->physics.speed_y++;
        enemy->timer_y = (uint32_t)game->timer;
    }
    enemy->physics.pos_y += enemy->physics.speed_y;
}


/**
 * @brief Check for a collision between the given enemy and the player.
 * 
 * @param game Pointer to the current game map.
 * @param player Pointer to the player_t object.
 * @param enemy Pointer to the enemy_t object.
 * 
 * @return 1 if the enemy is killed, else 0.
 */
static uint8_t check_enemy_player_collision(game_t *game, player_t *player, enemy_t *enemy)
{
    if (player->physics.pos_x < enemy->physics.pos_x + BLOCK_SIZE &&
        player->physics.pos_x > enemy->physics.pos_x - BLOCK_SIZE &&
        player->physics.pos_y < enemy->physics.pos_y + BLOCK_SIZE &&
        player->physics.pos_y > enemy->physics.pos_y - BLOCK_SIZE + KILL_ZONE_Y) {
        // The player has been hit
        if (player->shield) {
            player->shield = 0;
            enemy->life--;
        }
        else {
            player->life--;
            game->reset = 1;
        }
    }
    else if (player->physics.pos_x < enemy->physics.pos_x + BLOCK_SIZE &&
                player->physics.pos_x > enemy->physics.pos_x - BLOCK_SIZE &&
                player->physics.pos_y < enemy->physics.pos_y - BLOCK_SIZE + KILL_ZONE_Y &&
                player->physics.pos_y > enemy->physics.pos_y - BLOCK_SIZE) {
        // The enemy has been hit
        enemy->life--;
        initiate_jump(&player->physics, SPEED_INITIAL);
        player->timer = (uint32_t)game->timer;
        return 1;
    }
    return 0;
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


void compute_enemy(game_t *game, player_t *player, enemy_t *enemy, music_t **music)
{
    if (game == NULL) {
        printf("Error(update_enemy): game_t pointer is NULL.\n");
        assert(game);
    }
    if (player == NULL) {
        printf("Error(update_enemy): player_t pointer is NULL.\n");
        assert(player);
    }
    if (enemy == NULL) {
        printf("Error(update_enemy): enemy_t pointer is NULL.\n");
        assert(enemy);
    }
    if (music == NULL) {
        printf("Error(update_enemy): music_t pointer is NULL.\n");
        assert(music);
    }

    if (update_enemy_state(enemy)) {
        return;
    }
    update_enemy_position(game, enemy);
    if (check_enemy_player_collision(game, player, enemy)) {
        cue_music(music, NULL, game->map->data[enemy->row][enemy->column]);
    }
    // Check for lightstaff's usage from the player
    if (player->lightstaff && player->power_used) {
        uint8_t dist_x = abs(player->physics.pos_x - enemy->physics.pos_x);
        uint8_t dist_y = abs(player->physics.pos_y - enemy->physics.pos_y);
        if (hypot(dist_x, dist_y) < player->spell_radius) {
            enemy->life--;
        }
    }
    // Check for block collisions and update position if necessary
    if (check_block_collisions(game->map, &enemy->physics, NULL, game->cam_row)) {
        apply_reactive_force(&enemy->physics);
    }
    // Check if standing on a platform
    for (uint8_t i = 0; i < MAX_PLATFORMS; i++) {
        // Abort if no more platforms to check
        if (platforms[i].start_row == -1 || 
            enemy->physics.pos_x < (platforms[i].start_row - 1) * BLOCK_SIZE) {
            break;
        }
        if (!check_platform_collision(&enemy->physics, &platforms[i])) {
            continue;
        }
        // Collision with platform = "reactive force"
        enemy->physics.pos_y -= enemy->physics.speed_y;
        // If the platform moved, follow its movement
        if (platforms[i].horizontal && platforms[i].moved && !platforms[i].changed_dir) {
            enemy->physics.pos_x += platforms[i].physics.speed_x;
        }
        else if (platforms[i].horizontal && platforms[i].moved && platforms[i].changed_dir) {
            enemy->physics.pos_x += 2 * platforms[i].physics.speed_x;
        }
        else if (platforms[i].vertical && platforms[i].moved) {
            enemy->physics.pos_y += platforms[i].physics.speed_y;
        }
    }
    // Initiate shoot if possible
    if (game->map->data[enemy->row][enemy->column] == ENEMY_3 && !enemy->physics.falling &&
        ((game->timer - enemy->timer_x) / COOLDOWN_SHOOT)) {
        // Prepare the projectile
        if (is_on_sight(game->map, &enemy->physics, &player->physics)) {
            shoot_projectile(&enemy->physics, &player->physics);
        }
        enemy->timer_x = (uint32_t)game->timer;
    }
}
