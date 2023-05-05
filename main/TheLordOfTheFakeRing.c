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
    uint8_t map_id;
    uint16_t map_x;
    uint16_t map_row;
    uint64_t timer;
} game = {
    .reset      = 0,
    .playing    = 0,
    .cam_moving = 0,
    .map_id     = 0,
    .map_x      = 0,    // First pixel x-coordinate of the current map frame
    .map_row    = 1,    // First row of the current map frame
    .timer      = 0
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
    button_t button_C;
    gamepad_init_button(&button_C, PIN_BTN_C);
#pragma endregion


    /*************************************************
     * Game initialization
     *************************************************/
    // Setup game text objects
    const char coins_text[] = "COINS: ";
    const text_t coins_text_object = {
        .pos_x = 5,
        .pos_y = 5,
        .size = sizeof(coins_text),
        .data = coins_text,
        .color = BLACK
    };
    char num_coins_text[4] = {'\0'};
    const text_t coins_object = {
        .pos_x = 5 + sizeof(coins_text) * FONT_SIZE,
        .pos_y = 5,
        .size = sizeof(num_coins_text),
        .data = num_coins_text,
        .color = BLACK
    };
    const char life_text[] = "LIFE: ";
    const text_t life_text_object = {
        .pos_x = LCD_WIDTH / 2,
        .pos_y = 5,
        .size = sizeof(life_text),
        .data = life_text,
        .color = BLACK
    };
    char num_life_text[3] = {'\0'};
    const text_t life_object = {
        .pos_x = LCD_WIDTH / 2 + sizeof(life_text) * FONT_SIZE,
        .pos_y = 5,
        .size = sizeof(num_life_text),
        .data = num_life_text,
        .color = BLACK
    };

    const int8_t (*map)[NUM_BLOCKS_Y] = shire;
    game.map_id = MAP_ID(map);
    character_t player = {
        .life               = 3,
        .physics.pos_x      = BLOCK_SIZE,
        .physics.pos_y      = BLOCK_SIZE,
        .physics.speed_x    = SPEED_INITIAL,
        .physics.speed_y    = SPEED_INITIAL,
        .sprite.height      = BLOCK_SIZE,
        .sprite.width       = BLOCK_SIZE,
        .sprite.data        = sprite_player
    };

    initialize_blocks_records();
    // Create the enemies appearing in the very first frame of the map
    spawn_enemies(map, 1, NUM_BLOCKS_X + 1, game.map_row);

    while(1) {
        // Get the time for the current iteration
        ESP_ERROR_CHECK(gptimer_get_raw_count(timer_handle, &game.timer));
        game.timer = (uint64_t)game.timer / 10; // Convert to milliseconds


        /*************************************************
         * Game state check
         *************************************************/
        if (player.life == 0) {
            break;
        }
        else if (game.reset) {
            game.reset = 0;
            game.cam_moving = 0;
            game.map_x = 0;
            game.map_row = 1;
            // Reset character state (except life!)
            player.physics.pos_x                = BLOCK_SIZE;
            player.physics.pos_y                = LCD_HEIGHT - 2 * BLOCK_SIZE;
            player.physics.speed_x              = SPEED_INITIAL;
            player.physics.speed_y              = SPEED_INITIAL;
            player.physics.falling              = 0;
            player.physics.jumping              = 0;
            player.physics.top_collision        = 0;
            player.physics.bottom_collision     = 0;
            player.physics.left_collision       = 0;
            player.physics.right_collision      = 0;
            player.firestaff                    = 0;
            player.shield                       = 0;
            player.timer                        = 0;
            // Initialize enemy records
            for (uint8_t i = 0; i < NUM_ENEMY_RECORDS; i++) {
                memset(&enemies[i], 0, sizeof(enemies[i]));
            }
            // Re-spawn first enemies
            spawn_enemies(map, 1, NUM_BLOCKS_X + 1, game.map_row);

            ets_delay_us(1*1000*1000);
        }
        // Reset hit flag for each blocks
        for (uint8_t i = 0; i < NUM_BLOCK_RECORDS; i++) {
            blocks[i].is_hit = 0;
        }

        /*************************************************
         * Player state & position update
         *************************************************/
        // Update the player's x-position
        int8_t x = gamepad_read_joystick_axis(adc_handle, joystick.axis_x);
        player.physics.pos_x += player.physics.speed_x * (int16_t)(x / 100);
        if (player.physics.pos_x + BLOCK_SIZE / 2 > LCD_WIDTH / 2) {
            game.map_x += player.physics.speed_x; // Shift the map frame by 'speed_x' pixel
            game.map_row = (uint16_t)(game.map_x / BLOCK_SIZE) + 1;
            game.cam_moving = 1;
            player.physics.pos_x = LCD_WIDTH / 2 - BLOCK_SIZE / 2;
        }
        else if (player.physics.pos_x < 0) {
            player.physics.pos_x = 0;
        }
        else {
            game.cam_moving = 0;
        }

        // Check if the player is falling or jumping
        if (player.physics.falling && player.physics.bottom_collision) {
            player.physics.falling = 0;
            player.physics.speed_y = SPEED_INITIAL;
        }
        else if ((player.physics.bottom_collision || player.physics.left_collision ||
                 player.physics.right_collision) && gamepad_poll_button(&button_C)) {
            player.timer = (uint32_t)game.timer;
            player.physics.jumping = 1;
            player.physics.falling = 0;
            player.physics.speed_y = SPEED_JUMP_INIT;
        }
        else if ((player.physics.jumping && player.physics.top_collision) ||
                (player.physics.jumping && player.physics.speed_y == 0)) {
            player.physics.jumping = 0;
            player.physics.falling = 1;
        }
        else if (!player.physics.jumping && !player.physics.bottom_collision) {
            player.physics.falling = 1;
        }

        // Update the player's y-position
        player.physics.accelerating = (game.timer - player.timer) / TIMESTEP_ACCEL;
        if (player.physics.jumping) {
            if (player.physics.accelerating) {
                player.timer = (uint32_t)game.timer;
                player.physics.accelerating = 0;
                player.physics.speed_y--; // decelerating here
            }
            player.physics.pos_y -= player.physics.speed_y;
        }
        else {
            if (player.physics.falling && player.physics.accelerating) {
                player.timer = (uint32_t)game.timer;
                player.physics.accelerating = 0;
                player.physics.speed_y++; // accelerating here
            }
            player.physics.pos_y += player.physics.speed_y;
        }
        if (LCD_HEIGHT < player.physics.pos_y) {
            player.life--;
            game.reset = 1;
        }


        /*************************************************
         * Enemies states & positions update
         *************************************************/
        // Check the next 4 rows for enemies to spawn
        spawn_enemies(map, game.map_row + NUM_BLOCKS_X, game.map_row + NUM_BLOCKS_X + 3,game.map_row);
        // Apply dynamics to each living enemy
        for (int i = 0; i < NUM_ENEMY_RECORDS; i++) {
            if (enemies[i].life == 0 || enemies[i].physics.pos_x < -BLOCK_SIZE) {
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
            // Update x-position
            if (game.cam_moving) {
                enemies[i].physics.pos_x -= 2 * enemies[i].physics.speed_x;
                enemies[i].timer_x = (uint32_t)game.timer;
            }
            else if ((game.timer - enemies[i].timer_x) / TIMESTEP_ENEMY > 1) {
                enemies[i].physics.pos_x -= enemies[i].physics.speed_x;
                enemies[i].timer_x = (uint32_t)game.timer;
            }
            // Update y-position
            enemies[i].physics.accelerating = (game.timer - enemies[i].timer_y) / TIMESTEP_ACCEL;
            if (enemies[i].physics.falling && enemies[i].physics.accelerating) {
                enemies[i].physics.accelerating = 0;
                enemies[i].physics.speed_y++;
                enemies[i].timer_y = (uint32_t)game.timer;
            }
            enemies[i].physics.pos_y += enemies[i].physics.speed_y;
        }


        /*************************************************
         * Collision checks
         *************************************************/
        // PLayer's collisions
        if (check_block_collisions(map, &player.physics, game.map_x)) {
            return;
        }
        // Enemies' collisions
        for (int i = 0; i < NUM_ENEMY_RECORDS; i++) {
            if (enemies[i].life == 0) {
                break;
            }
            if (check_block_collisions(map, &enemies[i].physics, game.map_x)) {
                return;
            }
            // Check for collision with player
            if (player.physics.pos_x < enemies[i].physics.pos_x + BLOCK_SIZE &&
                player.physics.pos_x > enemies[i].physics.pos_x - BLOCK_SIZE &&
                player.physics.pos_y < enemies[i].physics.pos_y + BLOCK_SIZE &&
                player.physics.pos_y > enemies[i].physics.pos_y - BLOCK_SIZE + KILL_ZONE_Y) {
                
                player.life--;
                game.reset = 1;
            }
            else if (player.physics.pos_x < enemies[i].physics.pos_x + BLOCK_SIZE &&
                     player.physics.pos_x > enemies[i].physics.pos_x - BLOCK_SIZE &&
                     player.physics.pos_y < enemies[i].physics.pos_y - BLOCK_SIZE + KILL_ZONE_Y &&
                     player.physics.pos_y > enemies[i].physics.pos_y - BLOCK_SIZE) {
                enemies[i].life--;
                player.physics.jumping = 1;
                player.physics.falling = 0;
                player.physics.speed_y = 1;
                player.timer = (uint32_t)game.timer;
            }
        }


        /*************************************************
         * Blocks states & items update
         *************************************************/
        for (uint8_t i = 0; i < NUM_BLOCK_RECORDS; i++) {
            if (blocks[i].row == -1 || blocks[i].column == -1) {
                continue;
            }
            // If the block has been hit, update its status based on its type
            int8_t block_type = map[blocks[i].row][blocks[i].column];
            if (blocks[i].is_hit) {
                switch (block_type) {
                    case BREAKABLE_BLOCK:
                        blocks[i].destroyed = 1;
                        blocks[i].is_hit = 0;
                        break;
                    case BONUS_BLOCK:
                        if (!blocks[i].item_given) {
                            // TODO: generate random item command here
                            item_t item = {
                                .spawned = 1,
                                .sprite.height = BLOCK_SIZE,
                                .sprite.width = BLOCK_SIZE,
                                .sprite.pos_x = BLOCK_SIZE * (blocks[i].row - 1) - game.map_x,
                                .sprite.pos_y = LCD_HEIGHT - (BLOCK_SIZE * (blocks[i].column + 1)),
                                .sprite.data = coin_sprite_1,
                                .animation.bumping = 1
                            };
                            store_item(item);
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
            // Update the x-position of each object with respect to the camera movement speed
            if (game.cam_moving) {
                items[i].sprite.pos_x -= player.physics.speed_x;
            }
        }


        /*************************************************
         * Frame construction and display
         *************************************************/
        /*
         Sequentially, draw the background, blocks, enemies, items and then player.
         Graphic assets drawn first will appear below other graphic assets.
        */ 
        st7735s_fill_background(MAP_BACKGROUND(map));
        // Draw the items
        for (int i = 0; i < NUM_ITEMS; i++) {
            if (!items[i].spawned) {
                continue;
            }
            else if (items[i].taken || items[i].sprite.pos_x < -BLOCK_SIZE) {
                memset(&items[i], 0, sizeof(items[i]));
                continue;
            }
            if (items[i].animation.bumping &&
                (game.timer - items[i].animation.timer) / TIMESTEP_BUMP_COIN > 1) {
                // Make the coin go up and down
                items[i].animation.steps++;
                if (items[i].animation.steps <= HEIGHT_BUMP_COIN) {
                    items[i].sprite.pos_y--;
                }
                else if (items[i].animation.steps <= HEIGHT_BUMP_COIN * 2 + 1) {
                    items[i].sprite.pos_y++;
                }
                else {
                    items[i].animation.bumping = 0;
                    items[i].animation.steps = 0;
                    items[i].taken = 1;
                    game.coins++;
                }
                // Animate the coin
                uint8_t next_sprite = (items[i].animation.steps % 3 == 1);
                if (next_sprite && items[i].sprite.data == coin_sprite_1) {
                    items[i].sprite.data = coin_sprite_2;
                }
                else if (next_sprite && items[i].sprite.data == coin_sprite_2 &&
                         !items[i].sprite.flip_x) {
                    items[i].sprite.data = coin_sprite_3;
                }
                else if (next_sprite && items[i].sprite.data == coin_sprite_3) {
                    items[i].sprite.data = coin_sprite_2;
                    items[i].sprite.flip_x = 1;
                }
                else if (next_sprite && items[i].sprite.data == coin_sprite_2 &&
                         items[i].sprite.flip_x) {
                    items[i].sprite.data = coin_sprite_1;
                    items[i].sprite.flip_x = 0;
                }
                items[i].animation.timer = (uint32_t)game.timer;
            }
            st7735s_draw_sprite(items[i].sprite);
        }
        // Draw the blocks
        for (uint8_t row = game.map_row; row <= game.map_row + NUM_BLOCKS_X; row++) {
            for (int column = 0; column < NUM_BLOCKS_Y; column++) {
                int8_t block_type = map[row][NUM_BLOCKS_Y - 1 - column];
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
                    .pos_x = (row - 1) * BLOCK_SIZE - game.map_x,
                    .pos_y = column * BLOCK_SIZE
                };
                switch (block_type) {
                    case NON_BREAKABLE_BLOCK:
                        switch (game.map_id) {
                            case 1: sprite.data = shire_block_1; break;
                            case 2: sprite.data = moria_block_1; break;
                            default: break;
                        }
                        break;
                    case BREAKABLE_BLOCK:
                        switch (game.map_id) {
                            case 1: sprite.data = shire_block_2; break;
                            case 2: sprite.data = moria_block_2; break;
                            default: break;
                        }
                        break;
                    case BONUS_BLOCK:
                        if (block != NULL && block->bumping) {
                            bump_block(block, &sprite, game.timer);
                        }
                        switch (game.map_id) {
                            case 1: sprite.data = shire_block_3; break;
                            case 2: sprite.data = moria_block_3; break;
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
        // Draw the enemies
        for (int i = 0; i < NUM_ENEMY_RECORDS; i++) {
            if (enemies[i].life == 0) {
                memset(&enemies[i], 0, sizeof(enemies[i]));
                break;
            }
            sprite_t sprite = {
                .height = BLOCK_SIZE,
                .width = BLOCK_SIZE,
                .pos_x = enemies[i].physics.pos_x,
                .pos_y = enemies[i].physics.pos_y
            };
            switch(map[enemies[i].row][enemies[i].column]) {
                case ENEMY_1:
                    switch (game.map_id) {
                        case 1: sprite.data = shire_enemy_1; break;
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
        sprintf(num_coins_text, "%i", (uint8_t)game.coins);
        st7735s_draw_text(coins_object);
        
        st7735s_draw_text(life_text_object);
        sprintf(num_life_text, "%i", (uint8_t)player.life);
        st7735s_draw_text(life_object);
        

        // Finally, draw the player
        player.sprite.pos_x = player.physics.pos_x;
        player.sprite.pos_y = player.physics.pos_y;
        st7735s_draw_sprite(player.sprite);

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