/**
 * @file TheLordOfTheFakeRing.c
 * @author valentin betbeze (valentin.betbeze@gmail.com)
 * @brief Main code of the game.
 * @date 2023-04-16
 * 
 * @note
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "project_config.h"
#include "gamepad.h"
#include "st7735s_hal.h"
#include "st7735s_graphics.h"
#include "game_engine.h"
#include "rom/ets_sys.h"
#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"
#include "driver/gptimer.h"


/**
 * @brief Global game state flags & variables.
 */
struct {
    uint8_t reset :         1;
    uint8_t playing :       1;
    uint8_t cam_moving :    1;
    uint8_t coins;
    uint16_t map_x;             // First pixel x-coordinate of the current map frame
    uint16_t map_row;           // First row of the current map frame
    uint64_t timer;
} game = {
    .playing    = 1,
    .map_row    = 126,
};


void app_main()
{
    /*************************************************
     * Program initialization
     *************************************************/
#pragma region
    // Initialize non-SPI GPIOs
    const gpio_config_t io_conf = {
        .pin_bit_mask = ((1 << PIN_LCD_DC) | (1 << PIN_LCD_RES)),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    // Initialize SPI
    const spi_bus_config_t spi_bus_cfg = {
        .mosi_io_num = PIN_LCD_SDA,
        .miso_io_num = -1,
        .sclk_io_num = PIN_LCD_SCK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI_LCD_HOST, &spi_bus_cfg, SPI_LCD_DMA));
    spi_device_handle_t tft_handle;
    const spi_device_interface_config_t spi_dev_cfg = {
        .clock_speed_hz = SPI_LCD_FREQUENCY,
        .mode = SPI_LCD_MODE,                            
        .spics_io_num = PIN_LCD_CS, 
        .queue_size = SPI_LCD_QSIZE,
        .flags = SPI_LCD_FLAGS,
        .command_bits = 0,
        .address_bits = 0,
        .dummy_bits = 0
    };
    ESP_ERROR_CHECK(spi_bus_add_device(SPI_LCD_HOST, &spi_dev_cfg, &tft_handle));

    // Initialize LCD display
    st7735s_init_tft(tft_handle);
    st7735s_fill_background(BLACK);
    st7735s_push_frame(tft_handle);
    st7735s_init_pwm_backlight();
    st7735s_set_backlight(50);

    // Initialize timer instance
    const gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 10000 // 0.1 ms resolution
    };
    gptimer_handle_t timer_handle;
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &timer_handle));
    ESP_ERROR_CHECK(gptimer_enable(timer_handle));
    ESP_ERROR_CHECK(gptimer_start(timer_handle));

    // Create and initialize joystick
    joystick_t joystick = gamepad_create_joystick(JOY_MID_X, JOY_MID_Y);
    adc_oneshot_unit_handle_t adc_handle;
    gamepad_init_joystick(&adc_handle, &joystick);

    // Create and initialize buttons
    button_t button_A, button_C;
    gamepad_init_button(&button_A, PIN_BTN_A);
    gamepad_init_button(&button_C, PIN_BTN_C);
#pragma endregion

    /*************************************************
     * Game initialization
     *************************************************/
#pragma region
    game.map_x = game.map_row * BLOCK_SIZE; // Initialization
    map_t map = map_moria;

    // Setup UI
    const char coins_text[] = "COIN: ";
    const text_t coins_text_object = {
        .pos_x = 5,
        .pos_y = 5,
        .size = sizeof(coins_text),
        .data = coins_text,
        .color = BLACK,
        .adaptive = 1
    };
    char num_coins_text[4] = {'\0'};
    const text_t coins_object = {
        .pos_x = 5 + sizeof(coins_text) * FONT_SIZE,
        .pos_y = 5,
        .size = sizeof(num_coins_text),
        .data = num_coins_text,
        .color = BLACK,
        .adaptive = 1
    };
    const char life_text[] = "LIFE: ";
    const text_t life_text_object = {
        .pos_x = LCD_WIDTH / 2,
        .pos_y = 5,
        .size = sizeof(life_text),
        .data = life_text,
        .color = BLACK,
        .adaptive = 1
    };
    char num_life_text[3] = {'\0'};
    const text_t life_object = {
        .pos_x = LCD_WIDTH / 2 + sizeof(life_text) * FONT_SIZE,
        .pos_y = 5,
        .size = sizeof(num_life_text),
        .data = num_life_text,
        .color = BLACK,
        .adaptive = 1
    };

    // Initialize and load game elements
    initialize_blocks_records();
    if (load_platforms(map)) {
        return;
    }
    spawn_enemies(map, game.map_x, game.map_row, game.map_row + NUM_BLOCKS_X + 1);

    // Create the player's character
    character_t player = {
        .life               = 3,
        .lightstaff         = 0,
        .shield             = 0,
        .forward            = 1,
        .physics.platform_i = -1,
        .physics.pos_x      = game.map_x + 0,
        .physics.pos_y      = 6 * BLOCK_SIZE,
        .physics.speed_x    = SPEED_INITIAL,
        .physics.speed_y    = SPEED_INITIAL,
        .sprite.height      = BLOCK_SIZE,
        .sprite.width       = BLOCK_SIZE,
        .sprite.data        = sprite_player
    };
#pragma endregion

    while(game.playing) {
        // Get the time for the current iteration
        ESP_ERROR_CHECK(gptimer_get_raw_count(timer_handle, &game.timer));
        game.timer = (uint64_t)game.timer / 10; // Convert to milliseconds

        /*************************************************
         * Game state check
         *************************************************/
#pragma region
        if (player.life == 0) {
            game.playing = 0;
            break;
        }
        else if (game.reset) {
            game.reset = 0;
            game.cam_moving = 0;
            game.map_x = 0;
            game.map_row = 0;
            // Reset character state (except life!)
            player.shield                       = 0;
            player.lightstaff                   = 0;
            player.forward                      = 1;
            player.power_used                   = 0;
            player.spell_radius                 = 0;
            player.timer                        = 0;
            player.coins                        = 0;
            player.physics.pos_x                = 0;
            player.physics.pos_y                = BLOCK_SIZE;
            player.physics.speed_x              = SPEED_INITIAL;
            player.physics.speed_y              = SPEED_INITIAL;
            player.physics.falling              = 0;
            player.physics.jumping              = 0;
            player.physics.top_collision        = 0;
            player.physics.bottom_collision     = 0;
            player.physics.left_collision       = 0;
            player.physics.right_collision      = 0;
            // Reset block states
            initialize_blocks_records();
            // Reset enemy records
            for (uint8_t i = 0; i < NUM_ENEMY_RECORDS; i++) {
                memset(&enemies[i], 0, sizeof(enemies[i]));
            }
            // Reset first enemies
            spawn_enemies(map, game.map_x, game.map_row, game.map_row + NUM_BLOCKS_X);
            // Reset items
            for (uint8_t i = 0; i < NUM_ITEMS; i++) {
                memset(&items[i], 0, sizeof(items[i]));
            }
            ets_delay_us(1*1000*1000);
        }
        // Reset hit flag for each blocks
        for (uint8_t i = 0; i < NUM_BLOCK_RECORDS; i++) {
            blocks[i].is_hit = 0;
        }
#pragma endregion


        /*************************************************
         * Platforms
         *************************************************/
        // Update the position of all platforms
        for (uint8_t i = 0; i < MAX_PLATFORMS; i++) {
            // Abort if no more platforms to update
            platforms[i].moved = 0;
            platforms[i].changed_dir = 0;
            if (platforms[i].start_row == -1) {
                break;
            }
            // update y
            else if (platforms[i].vertical && (game.timer - platforms[i].timer) / TIMESTEP_PLATFORM > 1) {
                uint8_t start_y = LCD_HEIGHT - (platforms[i].start_column + 1) * BLOCK_SIZE;
                uint8_t end_y = LCD_HEIGHT - (platforms[i].end_column + 1) * BLOCK_SIZE;
                if (start_y < platforms[i].physics.pos_y || platforms[i].physics.pos_y <= end_y) {
                    platforms[i].physics.speed_y *= -1;
                    platforms[i].changed_dir = 1;
                }
                platforms[i].physics.pos_y += platforms[i].physics.speed_y;
                platforms[i].timer = (uint32_t)game.timer;
                platforms[i].moved = 1;
            }
            // update x
            else if (platforms[i].horizontal && (game.timer - platforms[i].timer) / TIMESTEP_PLATFORM > 1) {
                if ((platforms[i].physics.pos_x < platforms[i].start_row * BLOCK_SIZE) ||
                    (platforms[i].end_row * BLOCK_SIZE <= platforms[i].physics.pos_x)) {
                    platforms[i].physics.speed_x *= -1;
                    platforms[i].changed_dir = 1;
                }
                platforms[i].physics.pos_x += platforms[i].physics.speed_x;
                platforms[i].timer = (uint32_t)game.timer;
                platforms[i].moved = 1;
            }
        }


        /*************************************************
         * Player state & position update
         *************************************************/
#pragma region
        // Check states (falling/jumping)
        if (player.physics.falling && player.physics.bottom_collision) {
            player.physics.falling = 0;
            player.physics.speed_y = SPEED_INITIAL;
        }
        else if ((player.physics.bottom_collision || player.physics.left_collision ||
                 player.physics.right_collision) && gamepad_poll_button(&button_C)) {
            player.timer = (uint32_t)game.timer;
            if (initiate_jump(&player.physics, SPEED_JUMP_INIT)) {
                return;
            }
        }
        else if ((player.physics.jumping && player.physics.top_collision) ||
                (player.physics.jumping && player.physics.speed_y == 0)) {
            player.physics.jumping = 0;
            player.physics.falling = 1;
        }
        else if (!player.physics.jumping && !player.physics.bottom_collision) {
            player.physics.falling = 1;
        }
        // Update x-position
        int8_t x = gamepad_read_joystick_axis(adc_handle, joystick.axis_x);
        if (0 < x / 100) {
            player.forward = 1;
        }
        else if (x / 100 < 0){
            player.forward = 0;
        }
        player.physics.pos_x += player.physics.speed_x * (int16_t)(x / 100);
        if (player.physics.pos_x - game.map_x + BLOCK_SIZE / 2 > LCD_WIDTH / 2) {
            game.cam_moving = 1;
            game.map_x += player.physics.speed_x; // Shift the map frame by 'speed_x' pixels
            game.map_row = (uint16_t)(game.map_x / BLOCK_SIZE);
        }
        else if (player.physics.pos_x < game.map_x) {
            player.physics.pos_x = game.map_x;
        }
        else {
            game.cam_moving = 0;
        }
        // Update y-position
        player.physics.accelerating = (game.timer - player.timer) / TIMESTEP_ACCEL;
        if (player.physics.jumping) {
            if (player.physics.accelerating) {
                player.timer = (uint32_t)game.timer;
                player.physics.accelerating = 0;
                player.physics.speed_y--; // Decelerating (due to gravity)
            }
            player.physics.pos_y -= player.physics.speed_y;
        }
        else {
            if (player.physics.falling && player.physics.accelerating &&
                player.physics.speed_y < BLOCK_SIZE / 2) {
                player.timer = (uint32_t)game.timer;
                player.physics.accelerating = 0;
                player.physics.speed_y++;
            }
            player.physics.pos_y += player.physics.speed_y; // Gravity
        }
        // Check for block collisions & update position if necessary
        int8_t collision =  check_block_collisions(map, &player.physics, game.map_row);
        if (collision == 1) {
            fix_block_collision(&player.physics);
        }
        else if (collision == -1) {
            return;
        }
        // Check if standing on a platform
        collision = check_platform_collision(&player.physics);
        if (collision == 1) {
            // Collision with platform = "reactive force"
            player.physics.pos_y -= player.physics.speed_y;
            // If moving, follow the movement of the platform
            if (platforms[player.physics.platform_i].moved && platforms[player.physics.platform_i].horizontal) {
                player.physics.pos_x += platforms[player.physics.platform_i].physics.speed_x;
            }
            else if (platforms[player.physics.platform_i].moved && platforms[player.physics.platform_i].vertical) {
                player.physics.pos_y += platforms[player.physics.platform_i].physics.speed_y;
            }
        }
        else if (collision == -1) {
            return;
        }
        // Check for lightstaff usage
        if (player.lightstaff && gamepad_poll_button(&button_A)) {
            player.power_used = 1;
        }
        // Check if the player fell
        if (LCD_HEIGHT + BLOCK_SIZE <= player.physics.pos_y) {
            player.life--;
            game.reset = 1;
        }
#pragma endregion


        /*************************************************
         * Enemies states & positions update
         *************************************************/
        // Check the next 4 rows for enemies to spawn
        spawn_enemies(map, game.map_x, game.map_row + NUM_BLOCKS_X + 1, game.map_row + NUM_BLOCKS_X + 3);
        // Apply dynamics to each living enemy
        for (int i = 0; i < NUM_ENEMY_RECORDS; i++) {
            // Kill & delete (non-grounded only) dead enemies from the record log
            if (enemies[i].life == 0 || LCD_HEIGHT < enemies[i].physics.pos_y) {
                if (!enemies[i].physics.grounded) {
                    memset(&enemies[i], 0, sizeof(enemies[i]));
                }
                enemies[i].life = 0;
                continue;
            }
            // Check if falling
            if (!enemies[i].physics.bottom_collision) {
                enemies[i].physics.falling = 1;
            }
            else if (enemies[i].physics.falling && enemies[i].physics.bottom_collision) {
                enemies[i].physics.falling = 0;
                enemies[i].physics.speed_y = SPEED_INITIAL;
            }
            // Update x-direction
            uint8_t turn_around =   (!enemies[i].physics.falling && 
                                    (enemies[i].physics.left_collision || enemies[i].physics.right_collision));
            if (turn_around) {
                enemies[i].physics.speed_x *= -1;
            }
            // Update x-position
            if (!enemies[i].stationary && 
                (((game.timer - enemies[i].timer_x) / TIMESTEP_ENEMY > 1) || turn_around)) {
                enemies[i].timer_x = (uint32_t)game.timer;
                enemies[i].physics.pos_x += enemies[i].physics.speed_x;
            }
            // Update y-position
            enemies[i].physics.accelerating = (game.timer - enemies[i].timer_y) / TIMESTEP_ACCEL;
            if (enemies[i].physics.falling && enemies[i].physics.accelerating) {
                enemies[i].physics.accelerating = 0;
                enemies[i].physics.speed_y++;
                enemies[i].timer_y = (uint32_t)game.timer;
            }
            enemies[i].physics.pos_y += enemies[i].physics.speed_y;
            // Check for block collisions and update position if necessary
            int8_t collision = check_block_collisions(map, &enemies[i].physics, game.map_row);
            if (collision == 1) {
                fix_block_collision(&enemies[i].physics);
            }
            else if (collision == -1) {
                return;
            }
            // Check if standing on a platform
            collision = check_platform_collision(&enemies[i].physics);
            if (collision == 1) {
                uint8_t index = enemies[i].physics.platform_i;
                // Collision with platform = "reactive force"
                enemies[i].physics.pos_y -= enemies[i].physics.speed_y;
                // If the platform moved, follow its movement
                if (platforms[index].horizontal && platforms[index].moved && !platforms[index].changed_dir) {
                    enemies[i].physics.pos_x += platforms[index].physics.speed_x;
                }
                else if (platforms[index].horizontal && platforms[index].moved && platforms[index].changed_dir) {
                    enemies[i].physics.pos_x += 2 * platforms[index].physics.speed_x;
                }
                else if (platforms[index].vertical && platforms[index].moved) {
                    enemies[i].physics.pos_y += platforms[index].physics.speed_y;
                }
            }
            else if (collision == -1) {
                return;
            }
            // Check for collision with player
            if (player.physics.pos_x < enemies[i].physics.pos_x + BLOCK_SIZE &&
                player.physics.pos_x > enemies[i].physics.pos_x - BLOCK_SIZE &&
                player.physics.pos_y < enemies[i].physics.pos_y + BLOCK_SIZE &&
                player.physics.pos_y > enemies[i].physics.pos_y - BLOCK_SIZE + KILL_ZONE_Y) {
                // The player has been hit
                if (player.shield) {
                    player.shield = 0;
                    enemies[i].life--;
                }
                else {
                    player.life--;
                    game.reset = 1;
                    break;
                }
            }
            else if (player.physics.pos_x < enemies[i].physics.pos_x + BLOCK_SIZE &&
                     player.physics.pos_x > enemies[i].physics.pos_x - BLOCK_SIZE &&
                     player.physics.pos_y < enemies[i].physics.pos_y - BLOCK_SIZE + KILL_ZONE_Y &&
                     player.physics.pos_y > enemies[i].physics.pos_y - BLOCK_SIZE) {
                // The enemy has been hit
                enemies[i].life--;
                if (initiate_jump(&player.physics, SPEED_INITIAL)) {
                    return;
                }
                player.timer = (uint32_t)game.timer;
            }
            // Check for lightstaff's usage
            if (player.lightstaff && player.power_used) {
                uint8_t dist_x = abs(player.physics.pos_x - enemies[i].physics.pos_x);
                uint8_t dist_y = abs(player.physics.pos_y - enemies[i].physics.pos_y);
                if (hypot(dist_x, dist_y) < player.spell_radius) {
                    enemies[i].life--;
                }
            }
            // Initiate shoot if possible
            if (enemies[i].stationary && !enemies[i].physics.falling &&
                ((game.timer - enemies[i].timer_x) / COOLDOWN_SHOOT)) {
                // Prepare the projectile
                int8_t on_sight = is_on_sight(map, &enemies[i].physics, &player.physics);
                if (on_sight == 1) {
                    if (initiate_shoot(&enemies[i].physics, &player.physics)) {
                        return;
                    }
                }
                else if (on_sight == -1) {
                    return;
                }
                enemies[i].timer_x = (uint32_t)game.timer;
            }
        }


        /*************************************************
         * projectiles
         *************************************************/
        for (uint8_t i = 0; i < MAX_PROJECTILES; i++) {
            if (projectiles[i].physics.speed_x == 0) {
                continue;
            }
            // Update position
            projectiles[i].physics.pos_x += projectiles[i].physics.speed_x;
            projectiles[i].physics.pos_y = projectiles[i].slope * projectiles[i].physics.pos_x + projectiles[i].offset;
            // Check for collision with the environment
            int8_t collision = check_block_collisions(map, &projectiles[i].physics, game.map_row);
            if (collision == 1) {
                memset(&projectiles[i], 0, sizeof(projectiles[i]));
            }
            else if (collision == -1) {
                return;
            }
            // Check for collision with the player
            if (player.physics.pos_x < projectiles[i].physics.pos_x + BLOCK_SIZE / 2 + HITBOX_PROJECTILE / 2 &&
                player.physics.pos_x > projectiles[i].physics.pos_x - BLOCK_SIZE / 2 - HITBOX_PROJECTILE / 2 &&
                player.physics.pos_y < projectiles[i].physics.pos_y + BLOCK_SIZE / 2 + HITBOX_PROJECTILE / 2 &&
                player.physics.pos_y > projectiles[i].physics.pos_y - BLOCK_SIZE / 2 - HITBOX_PROJECTILE / 2) {
                    memset(&projectiles[i], 0, sizeof(projectiles[i]));
                    player.life--;
                    game.reset = 1;
            }
        }

        /*************************************************
         * Items states & positions update
         *************************************************/
        // Collision between items and the player
        for (uint8_t i = 0; i < NUM_ITEMS; i++) {
            if (!items[i].spawned || items[i].taken) {
                continue;
            }
            else if (player.physics.pos_x - game.map_x < items[i].sprite.pos_x + items[i].sprite.width &&
                     player.physics.pos_x - game.map_x >= items[i].sprite.pos_x - items[i].sprite.width &&
                     player.physics.pos_y < items[i].sprite.pos_y + items[i].sprite.height &&
                     player.physics.pos_y >= items[i].sprite.pos_y - items[i].sprite.height) {
                if (items[i].type == LIGHTSTAFF) {
                    if (player.lightstaff) {
                        player.coins += 20;
                    }
                    player.lightstaff = 1;
                    items[i].taken = 1;
                }
                else if (items[i].type == SHIELD) {
                    if (player.shield) {
                        player.coins += 20;
                    }
                    player.shield = 1;
                    items[i].taken = 1;
                }
                break;
            }
            // Update the x-position of each object with respect to the camera movement speed
            if (items[i].spawned && game.cam_moving) {
                items[i].sprite.pos_x -= player.physics.speed_x;
            }
        }


        /*************************************************
         * Blocks states
         *************************************************/
        for (uint8_t i = 0; i < NUM_BLOCK_RECORDS; i++) {
            if (blocks[i].row == -1 || blocks[i].column == -1) {
                continue;
            }
            // If the block has been hit, update its status based on its type
            int8_t block_type = map.data[blocks[i].row][blocks[i].column];
            if (blocks[i].is_hit) {
                switch (block_type) {
                    case BREAKABLE_BLOCK:
                        blocks[i].destroyed = 1;
                        blocks[i].is_hit = 0;
                        break;
                    case BONUS_BLOCK:
                        if (!blocks[i].item_given) {
                            store_item(generate_item(blocks[i].row, blocks[i].column, game.map_x));
                            blocks[i].bumping = 1;
                            blocks[i].item_given = 1;
                        }
                        blocks[i].is_hit = 0;
                        break;
                    default:
                        printf("Warning(set_block_flags_on_hit): You're trying to change the state of a block that cannot interact with external events. Block type: %i\n", block_type);
                        break;
                }
            }
        }


        /*************************************************
         * Frame construction and display
         *************************************************/
        /*
         Sequentially, draw the background, blocks, enemies, items and then player.
         Graphic assets drawn first will appear below other graphic assets.
        */ 
        st7735s_fill_background(map.background_color);
        // Draw items
        for (uint8_t i = 0; i < NUM_ITEMS; i++) {
            if (!items[i].spawned) {
                continue;
            }
            else if (items[i].taken || items[i].sprite.pos_x < -BLOCK_SIZE) {
                memset(&items[i], 0, sizeof(items[i]));
                continue;
            }
            switch (items[i].type) {
                case COIN:
                    // Animate the coin
                    if ((game.timer - items[i].timer) / TIMESTEP_BUMP_COIN < 1) {
                        break;
                    }
                    items[i].steps++;
                    if (items[i].steps <= HEIGHT_BUMP_COIN) {
                        items[i].sprite.pos_y--;
                    }
                    else if (items[i].steps <= HEIGHT_BUMP_COIN * 2 + 1) {
                        items[i].sprite.pos_y++;
                    }
                    else {
                        items[i].steps = 0;
                        items[i].taken = 1;
                        player.coins++;
                    }
                    // Change the coin sprite 
                    uint8_t next_sprite = (items[i].steps % 3 == 1);
                    if (next_sprite && items[i].sprite.data == sprite_coin_1) {
                        items[i].sprite.data = sprite_coin_2;
                    }
                    else if (next_sprite && items[i].sprite.data == sprite_coin_2 &&
                            !items[i].sprite.flip_x) {
                        items[i].sprite.data = sprite_coin_3;
                    }
                    else if (next_sprite && items[i].sprite.data == sprite_coin_3) {
                        items[i].sprite.data = sprite_coin_2;
                        items[i].sprite.flip_x = 1;
                    }
                    else if (next_sprite && items[i].sprite.data == sprite_coin_2 &&
                            items[i].sprite.flip_x) {
                        items[i].sprite.data = sprite_coin_1;
                        items[i].sprite.flip_x = 0;
                    }
                    items[i].timer = (uint32_t)game.timer;
                    break;

                case LIGHTSTAFF:
                    if (items[i].steps <= BLOCK_SIZE) {
                        items[i].steps++;
                        items[i].sprite.pos_y--;
                    }
                    break;

                case SHIELD:
                    if (items[i].steps <= BLOCK_SIZE) {
                        items[i].steps++;
                        items[i].sprite.pos_y--;
                    }
                    break;
                default:
                    break;
            }
            if (items[i].sprite.data != NULL) {
                st7735s_draw_sprite(items[i].sprite);
            }
        }
        // Draw blocks
        for (uint8_t row = game.map_row; row <= game.map_row + NUM_BLOCKS_X; row++) {
            for (int column = 0; column < NUM_BLOCKS_Y; column++) {
                int8_t block_type = map.data[row][NUM_BLOCKS_Y - 1 - column];
                if (block_type == BACKGROUND_BLOCK) {
                    continue;
                }
                block_t *block = get_block_record(row, NUM_BLOCKS_Y - 1 - column);
                if (block != NULL && block->destroyed) {
                    continue;
                }
                // Draw the element
                sprite_t sprite = {
                    .height = BLOCK_SIZE,
                    .width = BLOCK_SIZE,
                    .pos_x = row * BLOCK_SIZE - game.map_x,
                    .pos_y = column * BLOCK_SIZE
                };
                switch (block_type) {
                    case CUSTOM_SPRITE_3:
                        switch (map.id) {
                            case SHIRE: 
                                rectangle_t rectangle = {
                                    .height = BLOCK_SIZE + 4,
                                    .width = BLOCK_SIZE + 4,
                                    .pos_x = row * BLOCK_SIZE - game.map_x - 2,
                                    .pos_y = column * BLOCK_SIZE - 2,
                                    .color = BLACK
                                };
                                st7735s_draw_rectangle(rectangle);
                                break;
                            case MORIA:
                                circle_t circle = {
                                    .pos_y = column * BLOCK_SIZE + 6,
                                    .color = YELLOW_1,
                                    .alpha = 0.8,
                                    .radius = 10,
                                };
                                // Simple light animation
                                if (game.timer % 4 < 2) {
                                    circle.pos_x = row * BLOCK_SIZE - game.map_x + 7;
                                }
                                else {
                                    circle.pos_x = row * BLOCK_SIZE - game.map_x + 9;
                                }
                                st7735s_draw_circle(circle);
                                sprite.flip_x = 1;
                                sprite.data = sprite_torch;
                                break;
                            default:
                                break;
                        }
                        break;
                    case CUSTOM_SPRITE_2:
                        switch (map.id) {
                            case SHIRE:
                                rectangle_t rectangle = {
                                    .height = BLOCK_SIZE,
                                    .width = BLOCK_SIZE - 2,
                                    .pos_x = row * BLOCK_SIZE - game.map_x + 1,
                                    .pos_y = column * BLOCK_SIZE + 3,
                                    .color = BLACK
                                };
                                st7735s_draw_rectangle(rectangle);
                                sprite.data = moria_block_1;
                                break;
                            case MORIA:
                                circle_t circle = {
                                    .pos_y = column * BLOCK_SIZE + 6,
                                    .color = YELLOW_1,
                                    .alpha = 0.8,
                                    .radius = 10,
                                };
                                // Simple light animation
                                if (game.timer % 4 < 2) {
                                    circle.pos_x = row * BLOCK_SIZE - game.map_x + 7;
                                }
                                else {
                                    circle.pos_x = row * BLOCK_SIZE - game.map_x + 5;
                                }
                                st7735s_draw_circle(circle);
                                sprite.data = sprite_torch;
                                break;
                            default:
                                break;
                        }
                        break;
                    case CUSTOM_SPRITE_1:
                        switch (map.id) {
                            case SHIRE: sprite.data = shire_block_water; break;
                            case MORIA: sprite.data = shire_block_water; break;
                            default: break;
                        }
                        break;
                    case NON_BREAKABLE_BLOCK_1:
                        switch (map.id) {
                            case SHIRE: sprite.data = shire_block_1; break;
                            case MORIA: sprite.data = moria_block_1; break;
                            default: break;
                        }
                        break;
                    case NON_BREAKABLE_BLOCK_2:
                        switch (map.id) {
                            case SHIRE: sprite.data = shire_block_1_1; break;
                            default: break;
                        }
                        break;
                    case BREAKABLE_BLOCK:
                        switch (map.id) {
                            case SHIRE: sprite.data = shire_block_2; break;
                            case MORIA: sprite.data = moria_block_2; break;
                            default: break;
                        }
                        break;
                    case BONUS_BLOCK:
                        if (block != NULL && block->bumping) {
                            bump_block(block, &sprite, game.timer);
                        }
                        switch (map.id) {
                            case SHIRE: 
                                if (block != NULL && block->item_given) {
                                    sprite.data = shire_block_3_1;
                                }
                                else {
                                    sprite.data = shire_block_3;
                                }
                                break;
                            case MORIA:
                                if (block != NULL && block->item_given) {
                                    sprite.data = moria_block_3_1;
                                }
                                else {
                                    sprite.data = moria_block_3;
                                }
                                break;
                            default: break;
                        }
                        break;
                    default:
                        break;
                }
                if (sprite.data != NULL) {
                    st7735s_draw_sprite(sprite);
                }
            }
        }
        // Draw platforms
        for (uint8_t i = 0; i < MAX_PLATFORMS; i++) {
            if (platforms[i].start_row == -1) {
                break;
            }
            else if (platforms[i].physics.pos_x + 2 * BLOCK_SIZE < game.map_x ||
                     game.map_x + BLOCK_SIZE * NUM_BLOCKS_X <= platforms[i].physics.pos_x) {
                continue; // if not in the frame
            }
            sprite_t platform_left = {
                .height = BLOCK_SIZE,
                .width = BLOCK_SIZE,
                .pos_x = platforms[i].physics.pos_x - game.map_x,
                .pos_y = platforms[i].physics.pos_y
            };
            sprite_t platform_right = {
                .height = BLOCK_SIZE,
                .width = BLOCK_SIZE,
                .pos_x = platform_left.pos_x + BLOCK_SIZE,
                .pos_y = platform_left.pos_y
            };
            switch (map.id) {
                case MORIA:
                    platform_left.data = moria_platform_block_1;
                    platform_right.data = moria_platform_block_2;
                break;
            }
            if (platform_left.data != NULL && platform_right.data != NULL) {
                st7735s_draw_sprite(platform_left);
                st7735s_draw_sprite(platform_right);
            }
        }
        // Draw projectiles
        for (uint8_t i = 0; i < MAX_PROJECTILES; i++) {
            if (projectiles[i].physics.speed_x == 0) {
                continue;
            }
            sprite_t sprite = {
                .pos_x = projectiles[i].physics.pos_x - game.map_x,
                .pos_y = projectiles[i].physics.pos_y,
                .height = BLOCK_SIZE,
                .width = BLOCK_SIZE,
                .data = sprite_projectile,
            };
            if (projectiles[i].physics.speed_x < 0) {
                sprite.flip_x = 1;
            }
            st7735s_draw_sprite(sprite);
        }
        // Draw enemies
        for (uint8_t i = 0; i < NUM_ENEMY_RECORDS; i++) {
            if (enemies[i].life <= 0) {
                continue;
            }
            sprite_t sprite = {
                .height = BLOCK_SIZE,
                .width = BLOCK_SIZE,
                .pos_x = enemies[i].physics.pos_x - game.map_x,
                .pos_y = enemies[i].physics.pos_y
            };
            if (enemies[i].physics.speed_x > 0) {
                sprite.flip_x = 1;
            }
            switch(map.data[enemies[i].row][enemies[i].column]) {
                case ENEMY_1:
                    switch (map.id) {
                        case SHIRE: sprite.data = shire_enemy_1; break;
                        case MORIA: sprite.data = moria_enemy_1; break;
                        default: break;
                    }
                    break;
                case ENEMY_2:
                    switch (map.id) {
                        case MORIA: sprite.data = moria_enemy_2; break;
                        default: break;
                    }
                    break;
                case ENEMY_3:
                    switch (map.id) {
                        case MORIA: sprite.data = moria_enemy_2; break;
                        default: break;
                    }
                    break;
                default:
                    break;
            }
            if (sprite.data != NULL) {
                st7735s_draw_sprite(sprite);
            }
        }
        // Draw text
        st7735s_draw_text(coins_text_object);
        st7735s_draw_text(life_text_object);
        sprintf(num_coins_text, "%i", (uint8_t)game.coins + player.coins);
        sprintf(num_life_text, "%i", (uint8_t)player.life);
        st7735s_draw_text(coins_object);        
        st7735s_draw_text(life_object);
        // Finally, draw the player & its stuff
#pragma region
        player.sprite.pos_x = player.physics.pos_x - game.map_x;
        player.sprite.pos_y = player.physics.pos_y;
        if (player.shield) {
            rectangle_t shield_fill = {
                .pos_x = player.sprite.pos_x,
                .pos_y = player.sprite.pos_y,
                .width = BLOCK_SIZE,
                .height = BLOCK_SIZE,
                .color = WHITE,
                .alpha = SHIELD_ALPHA
            };
            st7735s_draw_rectangle(shield_fill);
            sprite_t shield_edge = {
                .height = BLOCK_SIZE,
                .width = BLOCK_SIZE,
                .pos_x = player.sprite.pos_x - BLOCK_SIZE,
                .pos_y = player.sprite.pos_y,
                .data = sprite_shield_edge,
                .alpha = SHIELD_ALPHA,
            };
            st7735s_draw_sprite(shield_edge);
            shield_edge.CW_90 = 1;
            shield_edge.pos_x += BLOCK_SIZE;
            shield_edge.pos_y -= BLOCK_SIZE;
            st7735s_draw_sprite(shield_edge);
            shield_edge.CW_90 = 0;
            shield_edge.ACW_90 = 1;
            shield_edge.pos_y += 2 * BLOCK_SIZE;
            st7735s_draw_sprite(shield_edge);
            shield_edge.ACW_90 = 0;
            shield_edge.flip_x = 1;
            shield_edge.pos_x += BLOCK_SIZE;
            shield_edge.pos_y -= BLOCK_SIZE;
            st7735s_draw_sprite(shield_edge);
        }
        if (player.forward) {
            player.sprite.flip_x = 0;
        }
        else {
            player.sprite.flip_x = 1;
        }
        st7735s_draw_sprite(player.sprite);
        if (player.lightstaff) {
            sprite_t lightstaff = {
                .width = BLOCK_SIZE,
                .height = BLOCK_SIZE,
                .pos_x = player.sprite.pos_x,
                .pos_y = player.sprite.pos_y,
                .data = sprite_lightstaff_equipped
            };
            if (!player.forward) {
                lightstaff.flip_x = 1;
            }
            st7735s_draw_sprite(lightstaff);
        }
        // Lightstaff power animation
        if (player.power_used && player.spell_radius < (uint16_t)LCD_SIZE) {
            player.spell_radius += 5;
            circle_t circle = {
                .pos_x = player.sprite.pos_x + BLOCK_SIZE / 2,
                .pos_y = player.sprite.pos_y + BLOCK_SIZE / 2,
                .radius = player.spell_radius,
                .thickness = 0,
                .color = WHITE
            };
            st7735s_draw_circle(circle);
        }
        else if (player.power_used && player.spell_radius < (uint16_t)LCD_SIZE + 100) {
            player.lightstaff = 0;
            player.spell_radius += 10;
            rectangle_t rectangle = {
                .width = LCD_WIDTH,
                .height = LCD_HEIGHT,
                .pos_x = 0,
                .pos_y = 0,
                .color = WHITE,
                .alpha = (float)(player.spell_radius - LCD_SIZE) / 100.0
            };
            st7735s_draw_rectangle(rectangle);
        }
        else {
            player.power_used = 0;
            player.spell_radius = 0;
        }
#pragma endregion

        st7735s_push_frame(tft_handle);

        /*************************************************
         * Music
         *************************************************/


        // Feed the task watchdog timer
        TIMERG0.wdtwprotect.wdt_wkey = TIMG_WDT_WKEY_VALUE;
        TIMERG0.wdtfeed.wdt_feed = 1;
        TIMERG0.wdtwprotect.wdt_wkey = 0;
    }
}