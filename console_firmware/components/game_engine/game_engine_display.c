#include "game_engine.h"
#include "sprites.h"

#define TIMESTEP_BUMP_BLOCK     5           // in milliseconds
#define HEIGHT_BUMP_BLOCK       3           // Bump height of a block, in pixels

/**
 * @brief Animate a coin by making it flip while bumping.
 * 
 * @param game Game flags.
 * @param player Player's character.
 * @param item Pointer of the item to animate.
 * 
 * @note The player is necessary to increase its count of coins.
 */
static void animate_coin(const game_t *game, player_t *player, item_t *item)
{
    if ((game->timer - item->timer) / TIMESTEP_BUMP_COIN < 1) {
        return;
    }
    item->steps++;
    if (item->steps <= HEIGHT_BUMP_COIN) {
        item->sprite.pos_y--;
    }
    else if (item->steps <= HEIGHT_BUMP_COIN * 2 + 1) {
        item->sprite.pos_y++;
    }
    else {
        item->steps = 0;
        item->taken = 1;
        player->coins++;
    }
    // Change the coin sprite 
    const uint8_t next_sprite = (item->steps % 3 == 1);
    if (item->sprite.data == NULL) {
        item->sprite.data = sprite_coin_1;
    }
    if (next_sprite && item->sprite.data == sprite_coin_1) {
        item->sprite.data = sprite_coin_2;
    }
    else if (next_sprite && item->sprite.data == sprite_coin_2 &&
            !item->sprite.flip_x) {
        item->sprite.data = sprite_coin_3;
    }
    else if (next_sprite && item->sprite.data == sprite_coin_3) {
        item->sprite.data = sprite_coin_2;
        item->sprite.flip_x = 1;
    }
    else if (next_sprite && item->sprite.data == sprite_coin_2 &&
            item->sprite.flip_x) {
        item->sprite.data = sprite_coin_1;
        item->sprite.flip_x = 0;
    }
    item->timer = (uint32_t)game->timer;
}


/**
 * @brief Draw the given item on the frame.
 * 
 * @param game Game flags.
 * @param player Player's character.
 * @param item Pointer of the item to draw.
 */
static void draw_item(const game_t *game, player_t *player, item_t *item)
{
    if (!item->spawned) {
        return;
    }
    else if (item->taken || item->sprite.pos_x < -BLOCK_SIZE) {
        memset(item, 0, sizeof(*item));
        return;
    }
    else if (game->cam_moving) {
        // Update the x-position of each object with respect to the camera movement speed
        item->sprite.pos_x -= player->physics.speed_x;
    }
    switch (item->type) {
        case COIN:
            animate_coin(game, player, item);
            break;
        case LIGHTSTAFF:
            item->sprite.data = sprite_lightstaff;
            if (item->steps <= BLOCK_SIZE) {
                item->steps++;
                item->sprite.pos_y--;
            }
            break;
        case SHIELD:
            item->sprite.data = sprite_shield;
            if (item->steps <= BLOCK_SIZE) {
                item->steps++;
                item->sprite.pos_y--;
            }
            break;
        default:
            break;
    }
    if (item->sprite.data != NULL) {
        st7735s_draw_sprite(&item->sprite);
    }
}


/**
 * @brief Animate the block by making it do a little bump in the air.
 * 
 * @param game Game flags.
 * @param block Block to bump.
 * @param sprite Sprite of the block.
 * 
 * @warning The sprite and block pointers shall correspond, else the function
 * will not work properly.
 */
static void bump_block(const game_t *game, block_t *block, sprite_t *sprite)
{
    static uint64_t t0 = 0;
    static uint8_t steps = 0;
    if ((game->timer - t0) / TIMESTEP_BUMP_BLOCK < 1) {
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
    t0 = game->timer;
}


/**
 * @brief Create a yellow spotlight effect over the specified location.
 * 
 * @param game Game flags.
 * @param x x-center of the spotlight (reference: map_x = 0).
 * @param y y-center of the spotlight (reference: display).
 * @param radius Spotlight radius, in pixels.
 */
static void create_spotlight(const game_t *game, const int16_t x, const int8_t y,
                             const uint8_t radius)
{
    circle_t circle = {
        .pos_y = y,
        .color = YELLOW_1,
        .alpha = 0.8,
        .radius = radius,
    };
    // Simple light animation
    if (game->timer % 4 < 2) {
        circle.pos_x = x - game->cam_pos_x + 7;
    }
    else {
        circle.pos_x = x - game->cam_pos_x + 9;
    }
    st7735s_draw_circle(&circle);
}


/**
 * @brief Animate a sprite (in this game, the ring), by making it spin over
 * a glowing halo of light.
 * 
 * @param game Game flags.
 * @param sprite Sprite to animate.
 */
static void animate_ring(const game_t *game, sprite_t *sprite)
{
    static item_t ring = {
        .sprite.data = sprite_ring_1,
    };
    if ((game->timer - ring.timer) / TIMESTEP_BUMP_COIN < 1) {
        return;
    }
    ring.steps++;
    circle_t circle = {
        .pos_x = sprite->pos_x + BLOCK_SIZE / 2,
        .pos_y = sprite->pos_y + BLOCK_SIZE / 2 - 1,
        .color = YELLOW_1,
        .alpha = 0.9,
        .radius = 20,
    };
    // Simple light animation
    if (ring.steps % 6 < 3) {
        circle.radius = 21;
    }
    else {
        circle.radius = 20;
    }
    st7735s_draw_circle(&circle);
    const uint8_t next_sprite = (ring.steps % 6 == 1);
    if (next_sprite && ring.sprite.data == sprite_ring_1) {
        ring.sprite.data = sprite_ring_2;
    }
    else if (next_sprite && ring.sprite.data == sprite_ring_2 &&
            !ring.sprite.flip_x) {
        ring.sprite.data = sprite_ring_3;
    }
    else if (next_sprite && ring.sprite.data == sprite_ring_3) {
        ring.sprite.data = sprite_ring_2;
        ring.sprite.flip_x = 1;
    }
    else if (next_sprite && ring.sprite.data == sprite_ring_2 &&
            ring.sprite.flip_x) {
        ring.sprite.data = sprite_ring_1;
        ring.sprite.flip_x = 0;
    }
    ring.timer = (uint32_t)game->timer;
    sprite->data = ring.sprite.data;
}


/**
 * @brief Draw a block on the frame.
 * 
 * @param game Game flags.
 * @param row Row of the block.
 * @param column Column of the block.
 */
static void draw_block(const game_t *game, const int16_t row, const int8_t column)
{
    if (game->map->data[row][NUM_BLOCKS_Y - 1 - column] == BACKGROUND_BLOCK) {
        return;
    }
    uint8_t block_index = 0, block_state_found = 0;
    sprite_t sprite = {
        .height = BLOCK_SIZE,
        .width = BLOCK_SIZE,
        .pos_x = row * BLOCK_SIZE - game->cam_pos_x,
        .pos_y = column * BLOCK_SIZE
    };
    // Check the state of the current block
    if (get_block_record(&block_index, row, NUM_BLOCKS_Y - 1 - column)) {
        block_state_found = 1;
        if (blocks[block_index].destroyed) {
            return;
        }
        else if (blocks[block_index].bumping) {
            bump_block(game, &blocks[block_index], &sprite);
        }
    }
    // Assign graphic asset(s) to the block
    switch (game->map->data[row][NUM_BLOCKS_Y - 1 - column]) {
        case CUSTOM_SPRITE_4:
            switch (game->map->id) {
                case MORIA:
                    const rectangle_t rectangle = {
                        .height = BLOCK_SIZE,
                        .width = BLOCK_SIZE - 1,
                        .pos_x = row * BLOCK_SIZE - game->cam_pos_x,
                        .pos_y = column * BLOCK_SIZE + 1,
                        .color = WHITE
                    };
                    st7735s_draw_rectangle(&rectangle);
                    break;
                default: break;
            }
            break;
        case CUSTOM_SPRITE_3:
            switch (game->map->id) {
                case SHIRE: 
                    const rectangle_t rectangle = {
                        .height = BLOCK_SIZE + 4,
                        .width = BLOCK_SIZE + 4,
                        .pos_x = row * BLOCK_SIZE - game->cam_pos_x - 2,
                        .pos_y = column * BLOCK_SIZE - 2,
                        .color = BLACK
                    };
                    st7735s_draw_rectangle(&rectangle);
                    break;
                case MORIA:
                    create_spotlight(game, row * BLOCK_SIZE, column * BLOCK_SIZE + 5, 10);
                    sprite.flip_x = 1;
                    sprite.data = sprite_torch;
                    break;
                default: break;    
            }
            break;
        case CUSTOM_SPRITE_2:
            switch (game->map->id) {
                case SHIRE:
                    const rectangle_t rectangle = {
                        .height = BLOCK_SIZE,
                        .width = BLOCK_SIZE - 2,
                        .pos_x = row * BLOCK_SIZE - game->cam_pos_x + 1,
                        .pos_y = column * BLOCK_SIZE + 3,
                        .color = BLACK
                    };
                    st7735s_draw_rectangle(&rectangle);
                    sprite.data = moria_block_1;
                    break;
                case MORIA:
                    create_spotlight(game, row * BLOCK_SIZE - 1, column * BLOCK_SIZE + 5, 10);
                    sprite.data = sprite_torch;
                    break;
                default: break;
            }
            break;
        case CUSTOM_SPRITE_1:
            switch (game->map->id) {
                case SHIRE: sprite.data = shire_block_water; break;
                case MORIA: 
                    sprite.data = shire_block_water; 
                    sprite.alpha = 0.5;
                    break;
                default: break;
            }
            break;
        case NON_BREAKABLE_BLOCK_1:
            switch (game->map->id) {
                case SHIRE: sprite.data = shire_block_1; break;
                case MORIA: sprite.data = moria_block_1; break;
                default: break;  
            }
            break;
        case NON_BREAKABLE_BLOCK_2:
            switch (game->map->id) {
                case SHIRE: sprite.data = shire_block_1_1; break;
                default: break;
            }
            break;
        case BREAKABLE_BLOCK:
            switch (game->map->id) {
                case SHIRE: sprite.data = shire_block_2; break;
                case MORIA: sprite.data = moria_block_2; break;
                default:
                    break;
            }
            break;
        case BONUS_BLOCK:
            switch (game->map->id) {
                case SHIRE: 
                    if (block_state_found && blocks[block_index].item_given) sprite.data = shire_block_3_1;
                    else sprite.data = shire_block_3;
                    break;
                case MORIA:
                    if (block_state_found && blocks[block_index].item_given) sprite.data = moria_block_3_1;
                    else sprite.data = moria_block_3;
                    break;
                default: break;
            }
            break;
        case RING:
            // Animate the ring
            if (blocks[block_index].item_given && !blocks[block_index].bumping) {
                blocks[block_index].destroyed = 1;
            }
            animate_ring(game, &sprite);
            break;
        default: break;
    }
    if (sprite.data != NULL) st7735s_draw_sprite(&sprite);
}


/**
 * @brief Draw a platform on the frame.
 * 
 * @param game Game flags.
 * @param platform Pointer of the platform to draw.
 */
static void draw_platform(const game_t *game, const platform_t *platform)
{
    if (platform->physics.pos_x + 2 * BLOCK_SIZE < game->cam_pos_x ||
        game->cam_pos_x + BLOCK_SIZE * NUM_BLOCKS_X <= platform->physics.pos_x) {
        return; // not in the frame
    }
    sprite_t platform_left = {
        .height = BLOCK_SIZE,
        .width = BLOCK_SIZE,
        .pos_x = platform->physics.pos_x - game->cam_pos_x,
        .pos_y = platform->physics.pos_y
    };
    sprite_t platform_right = {
        .height = BLOCK_SIZE,
        .width = BLOCK_SIZE,
        .pos_x = platform_left.pos_x + BLOCK_SIZE,
        .pos_y = platform_left.pos_y
    };
    switch (game->map->id) {
        case SHIRE:
            break;
        case MORIA:
            platform_left.data = moria_platform_block_1;
            platform_right.data = moria_platform_block_2;
        break;
    }
    if (platform_left.data != NULL && platform_right.data != NULL) {
        st7735s_draw_sprite(&platform_left);
        st7735s_draw_sprite(&platform_right);
    }
}


/**
 * @brief Draw a projectile on the frame.
 * 
 * @param game Game flags.
 * @param projectile Pointer of the projectile to draw.
 */
static void draw_projectile(const game_t *game, const projectile_t *projectile)
{
    if (projectile->physics.speed_x == 0) {
        return;
    }
    sprite_t sprite = {
        .pos_x = projectile->physics.pos_x - game->cam_pos_x,
        .pos_y = projectile->physics.pos_y,
        .height = BLOCK_SIZE,
        .width = BLOCK_SIZE,
        .data = sprite_projectile,
    };
    if (projectile->physics.speed_x < 0) {
        sprite.flip_x = 1;
    }
    st7735s_draw_sprite(&sprite);
}


/**
 * @brief Draw an enemy on the frame.
 * 
 * @param game Game flags.
 * @param enemy Pointer of the enemy to draw.
 */
static void draw_enemy(const game_t *game, const enemy_t *enemy)
{
    if (enemy->life <= 0) {
        return;
    }
    sprite_t sprite = {
        .height = BLOCK_SIZE,
        .width = BLOCK_SIZE,
        .pos_x = enemy->physics.pos_x - game->cam_pos_x,
        .pos_y = enemy->physics.pos_y
    };
    if (enemy->physics.speed_x > 0) {
        sprite.flip_x = 1;
    }
    switch(game->map->data[enemy->row][enemy->column]) {
        case ENEMY_1:
            switch (game->map->id) {
                case SHIRE: sprite.data = shire_enemy_1; break;
                case MORIA: sprite.data = moria_enemy_1; break;
                default: break;
            }
            break;
        case ENEMY_2:
            switch (game->map->id) {
                case MORIA: sprite.data = moria_enemy_2; break;
                default: break;
            }
            break;
        case ENEMY_3:
            switch (game->map->id) {
                case MORIA: sprite.data = moria_enemy_2; break;
                default: break;
            }
            break;
        case ENEMY_4:
            switch (game->map->id) {
                case MORIA: sprite.data = moria_enemy_1; break;
                default: break;
            }
            break;
        default:
            break;
    }
    if (sprite.data != NULL) {
        st7735s_draw_sprite(&sprite);
    }
}


uint8_t transition_screen(const uint16_t color, const uint8_t fade_in)
{
    static uint8_t steps = 0;
    rectangle_t rectangle = {
        .width = LCD_WIDTH,
        .height = LCD_HEIGHT,
        .pos_x = 0,
        .pos_y = 0,
        .color = color,
    };
    // On-going transition
    if (steps < 100) {
        if (fade_in) {
            rectangle.alpha = (float)(100 - steps) / 100;
        }
        else {
            rectangle.alpha = (float)steps / 100;
        }
        st7735s_draw_rectangle(&rectangle);
        steps += 2;
        return 0;
    }
    // Transition complete
    if (fade_in) {
        rectangle.alpha = 0;
    }
    else {
        rectangle.alpha = 1;
    }
    st7735s_draw_rectangle(&rectangle);
    steps = 0;
    return 1;
}


void draw_player(const game_t *game, player_t *player)
{
    if (game == NULL) {
        printf("Error(draw_player): game_t pointer is NULL.\n");
        assert(game);
    }
    if (player == NULL) {
        printf("Error(draw_player): player_t pointer is NULL.\n");
        assert(player);
    }
    player->sprite.pos_x = player->physics.pos_x - game->cam_pos_x;
    player->sprite.pos_y = player->physics.pos_y;
    if (player->shield) {
        const rectangle_t shield_fill = {
            .pos_x = player->sprite.pos_x,
            .pos_y = player->sprite.pos_y,
            .width = BLOCK_SIZE,
            .height = BLOCK_SIZE,
            .color = WHITE,
            .alpha = SHIELD_ALPHA
        };
        st7735s_draw_rectangle(&shield_fill);
        sprite_t shield_edge = {
            .height = BLOCK_SIZE,
            .width = BLOCK_SIZE,
            .pos_x = player->sprite.pos_x - BLOCK_SIZE,
            .pos_y = player->sprite.pos_y,
            .data = sprite_shield_edge,
            .alpha = SHIELD_ALPHA,
        };
        st7735s_draw_sprite(&shield_edge);
        shield_edge.CW_90 = 1;
        shield_edge.pos_x += BLOCK_SIZE;
        shield_edge.pos_y -= BLOCK_SIZE;
        st7735s_draw_sprite(&shield_edge);
        shield_edge.CW_90 = 0;
        shield_edge.ACW_90 = 1;
        shield_edge.pos_y += 2 * BLOCK_SIZE;
        st7735s_draw_sprite(&shield_edge);
        shield_edge.ACW_90 = 0;
        shield_edge.flip_x = 1;
        shield_edge.pos_x += BLOCK_SIZE;
        shield_edge.pos_y -= BLOCK_SIZE;
        st7735s_draw_sprite(&shield_edge);
    }
    if (player->forward) {
        player->sprite.flip_x = 0;
    }
    else {
        player->sprite.flip_x = 1;
    }
    st7735s_draw_sprite(&player->sprite);
    if (player->lightstaff) {
        sprite_t lightstaff = {
            .width = BLOCK_SIZE,
            .height = BLOCK_SIZE,
            .pos_x = player->sprite.pos_x,
            .pos_y = player->sprite.pos_y,
            .data = sprite_lightstaff_equipped
        };
        if (!player->forward) {
            lightstaff.flip_x = 1;
        }
        st7735s_draw_sprite(&lightstaff);
    }
    // Lightstaff power animation
    if (player->power_used && player->spell_radius < (uint16_t)LCD_SIZE) {
        player->spell_radius += 5;
        const circle_t circle = {
            .pos_x = player->sprite.pos_x + BLOCK_SIZE / 2,
            .pos_y = player->sprite.pos_y + BLOCK_SIZE / 2,
            .radius = player->spell_radius,
            .thickness = 0,
            .color = WHITE
        };
        st7735s_draw_circle(&circle);
    }
    else if (player->power_used && player->spell_radius < (uint16_t)LCD_SIZE + 100) {
        player->lightstaff = 0;
        player->spell_radius += 10;
        const rectangle_t rectangle = {
            .width = LCD_WIDTH,
            .height = LCD_HEIGHT,
            .pos_x = 0,
            .pos_y = 0,
            .color = WHITE,
            .alpha = (float)(player->spell_radius - LCD_SIZE) / 100.0
        };
        st7735s_draw_rectangle(&rectangle);
    }
    else {
        player->power_used = 0;
        player->spell_radius = 0;
    }
}


void build_frame(game_t *game, player_t *player)
{
    if (game == NULL) {
        printf("Error(build_frame): game_t pointer is NULL.\n");
        assert(game);
    }
    if (game->map == NULL) {
        printf("Error(build_frame): map_t pointer is NULL.\n");
        assert(game->map);
    }
    if (game->map->data == NULL) {
        printf("Error(build_frame): map.data pointer is NULL.\n");
        assert(game->map->data);
    }
    if (player == NULL) {
        printf("Error(build_frame): player_t pointer is NULL.\n");
        assert(player);
    }
    st7735s_fill_background(game->map->background_color);
    // Draw items
    for (uint8_t i = 0; i < NUM_ITEMS; i++) {
        draw_item(game, player, &items[i]);
    }
    // Draw blocks
    for (uint8_t row = game->cam_row; row <= game->cam_row + NUM_BLOCKS_X; row++) {
        if (game->map->nrows - 1 < row) {
            break;
        }
        for (int column = 0; column < NUM_BLOCKS_Y; column++) {
            draw_block(game, row, column);
        }
    }
    // Draw platforms
    for (uint8_t i = 0; i < MAX_PLATFORMS; i++) {
        if (platforms[i].start_row == -1) {
            break;
        }
        draw_platform(game, &platforms[i]);
    }
    // Draw projectiles
    for (uint8_t i = 0; i < MAX_PROJECTILES; i++) {
        draw_projectile(game, &projectiles[i]);
    }
    // Draw enemies
    for (uint8_t i = 0; i < NUM_ENEMY_RECORDS; i++) {
        draw_enemy(game, &enemies[i]);
    }
}
