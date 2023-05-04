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
     * Program body
     *************************************************/

    const int8_t (*map)[NUM_BLOCKS_Y] = shire;
    uint8_t map_id = MAP_ID(map);
    uint16_t map_x = 0;     // Left position of the current map frame, in pixels
    uint16_t map_row = 1;   // First row of the current map frame
    uint8_t camera_moving = 0;
    character_t player = {
        .life               = 3,
        .physics.pos_x      = 16,
        .physics.pos_y      = 0,
        .physics.speed_x    = INITIAL_SPEED,
        .physics.speed_y    = INITIAL_SPEED,
        .sprite.height      = BLOCK_SIZE,
        .sprite.width       = BLOCK_SIZE,
        .sprite.data        = sprite_player
    };

    // Create the enemies appearing in the very first frame of the map
    spawn_enemies(map, 1, NUM_BLOCKS_X + 1, map_row);

    while(1) {
        // Get the time for the current iteration
        uint64_t timer;
        ESP_ERROR_CHECK(gptimer_get_raw_count(timer_handle, &timer));
        timer = (uint64_t)timer / 10; // Convert to milliseconds

        camera_moving = 0;
        // Update the player's x-position
        int8_t x = gamepad_read_joystick_axis(adc_handle, joystick.axis_x);
        player.physics.pos_x += player.physics.speed_x * (int16_t)(x / 100);
        if (player.physics.pos_x + BLOCK_SIZE / 2 > LCD_WIDTH / 2) {
            map_x += player.physics.speed_x; // Shift the map frame by 'speed_x' pixel
            map_row = (uint16_t)(map_x / BLOCK_SIZE) + 1;
            player.physics.pos_x = LCD_WIDTH / 2 - BLOCK_SIZE / 2;
            camera_moving = 1;
        }
        else if (player.physics.pos_x < 0) {
            player.physics.pos_x = 0;
        }

        // Check if the player is falling or jumping
        if (player.physics.falling && player.physics.bottom_collision) {
            player.physics.falling = 0;
            player.physics.speed_y = INITIAL_SPEED;
        }
        else if ((player.physics.bottom_collision || player.physics.left_collision ||
                 player.physics.right_collision) && gamepad_poll_button(&button_C)) {
            player.timer = (uint32_t)timer;
            player.physics.jumping = 1;
            player.physics.falling = 0;
            player.physics.speed_y = JUMP_INIT_SPEED;
        }
        else if ((player.physics.jumping && player.physics.top_collision) ||
                (player.physics.jumping && player.physics.speed_y == 0)) {
            player.physics.jumping = 0;
            player.physics.falling = 1;
            player.physics.speed_y = 0;
        }
        else if (!player.physics.jumping &&
                 !player.physics.falling &&
                 !player.physics.bottom_collision) {
            player.physics.falling = 1;
        }

        // Update the player's y-position
        player.physics.accelerating = (timer - player.timer) / TIMESTEP_ACCEL;
        if (!player.physics.jumping) {
            if (player.physics.falling && player.physics.accelerating) {
                player.timer = (uint32_t)timer;
                player.physics.accelerating = 0;
                player.physics.speed_y++;
            }
            player.physics.pos_y += player.physics.speed_y;
        }
        else {
            if (player.physics.accelerating) {
                player.timer = (uint32_t)timer;
                player.physics.accelerating = 0;
                player.physics.speed_y--;
            }
            player.physics.pos_y -= player.physics.speed_y;
        }


        /*************************************************
         * Enemies
         *************************************************/
        // Check the next 4 rows for enemies to spawn
        spawn_enemies(map, map_row + NUM_BLOCKS_X, map_row + NUM_BLOCKS_X + 3, map_row);
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
                enemies[i].physics.speed_y = INITIAL_SPEED;
            }
            // Update x-position
            if (camera_moving) {
                enemies[i].physics.pos_x -= 2 * enemies[i].physics.speed_x;
                enemies[i].timer_x = (uint32_t)timer;
            }
            else if ((timer - enemies[i].timer_x) / TIMESTEP_ENEMY > 1) {
                enemies[i].physics.pos_x -= enemies[i].physics.speed_x;
                enemies[i].timer_x = (uint32_t)timer;
            }
            // Update y-position
            enemies[i].physics.accelerating = (timer - enemies[i].timer_y) / TIMESTEP_ACCEL;
            if (enemies[i].physics.falling && enemies[i].physics.accelerating) {
                enemies[i].physics.accelerating = 0;
                enemies[i].physics.speed_y++;
                enemies[i].timer_y = (uint32_t)timer;
            }
            enemies[i].physics.pos_y += enemies[i].physics.speed_y;
        }


        /*************************************************
         * Collision checks
         *************************************************/
        if (check_block_collisions(map, &player.physics, map_x)) {
            return;
        }
        
        for (int i = 0; i < NUM_ENEMY_RECORDS; i++) {
            if (enemies[i].life == 0) {
                break;
            }
            if (check_block_collisions(map, &enemies[i].physics, map_x)) {
                return;
            }
        }


        /*************************************************
         * Frame construction and display
         *************************************************/
        st7735s_fill_background(MAP_BACKGROUND(map));

        // Draw the current frame of the map
        for (uint8_t row = map_row; row <= map_row + NUM_BLOCKS_X; row++) {
            for (int column = 0; column < NUM_BLOCKS_Y; column++) {
                int8_t element_type = map[row][NUM_BLOCKS_Y - 1 - column];
                if (element_type == BACKGROUND_BLOCK) {
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
                    .pos_x = (row - 1) * BLOCK_SIZE - map_x,
                    .pos_y = column * BLOCK_SIZE
                };
                switch (element_type) {
                    case NON_BREAKABLE_BLOCK:
                        switch (map_id) {
                            case 1: sprite.data = shire_block_1; break;
                            case 2: sprite.data = moria_block_1; break;
                            default: break;
                        }
                        break;
                    case BREAKABLE_BLOCK:
                        switch (map_id) {
                            case 1: sprite.data = shire_block_2; break;
                            case 2: sprite.data = moria_block_2; break;
                            default: break;
                        }
                        break;
                    case BONUS_BLOCK:
                        if (block != NULL && block->bumping) {
                            bump(block, &sprite, timer);
                        }
                        switch (map_id) {
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

        // Draw the enemies (if any)
        for (int i = 0; i < NUM_ENEMY_RECORDS; i++) {
            if (enemies[i].life == 0) {
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
                    switch (map_id) {
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