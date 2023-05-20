/**
 * @file TheLordOfTheFakeRing.c
 * @author valentin betbeze (valentin.betbeze@gmail.com)
 * @brief Main code of the game.
 * @date 2023-04-16
 * 
 * @note Main source code file.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "rom/ets_sys.h"
#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"
#include "driver/gptimer.h"

#include "gamepad.h"
#include "st7735s_hal.h"
#include "st7735s_graphics.h"
#include "game_engine.h"
#include "fonts.h"
#include "maps.h"
#include "sprites.h"


void app_main()
{
    /*************************************************
     * Hardware initialization
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
    joystick_t joystick;
    gamepad_config_joystick(&joystick, JOY_MID_X, JOY_MID_Y);
    adc_oneshot_unit_handle_t adc_handle;
    gamepad_init_joystick(&adc_handle, &joystick);

    // Create and initialize buttons
    button_t button_A, button_C;
    gamepad_init_button(&button_A, PIN_BTN_A);
    gamepad_init_button(&button_C, PIN_BTN_C);
    #pragma endregion


    /*************************************************
     * Game parameters initialization
     *************************************************/
    #pragma region
    game_t game = {
        .playing = 1,
        .map = &map_shire
    };

    // Setup UI
    const char coins_text[] = "COIN: ";
    const text_t coins_text_object = {
        .pos_x = 5,
        .pos_y = 5,
        .size = sizeof(coins_text),
        .data = coins_text,
        .color = BLACK,
        .adaptive = 1,
        .font = myFont
    };
    char num_coins_text[4] = {'\0'};
    const text_t coins_object = {
        .pos_x = 5 + sizeof(coins_text) * FONT_SIZE,
        .pos_y = 5,
        .size = sizeof(num_coins_text),
        .data = num_coins_text,
        .color = BLACK,
        .adaptive = 1,
        .font = myFont
    };
    const char life_text[] = "LIFE: ";
    const text_t life_text_object = {
        .pos_x = LCD_WIDTH / 2,
        .pos_y = 5,
        .size = sizeof(life_text),
        .data = life_text,
        .color = BLACK,
        .adaptive = 1,
        .font = myFont
    };
    char num_life_text[3] = {'\0'};
    const text_t life_object = {
        .pos_x = LCD_WIDTH / 2 + sizeof(life_text) * FONT_SIZE,
        .pos_y = 5,
        .size = sizeof(num_life_text),
        .data = num_life_text,
        .color = BLACK,
        .adaptive = 1,
        .font = myFont
    };

    // Initialize and load game elements
    initialize_blocks_records();
    load_platforms(game.map);
    spawn_enemies(game.map, game.cam_pos_x, game.cam_row, game.cam_row + NUM_BLOCKS_X + 1);

    // Create the player's character
    player_t player = {
        .life               = 3,
        .lightstaff         = 0,
        .shield             = 0,
        .forward            = 1,
        .physics.platform_i = -1,
        .physics.pos_x      = game.map->start_row * BLOCK_SIZE,
        .physics.pos_y      = (NUM_BLOCKS_Y - game.map->start_column - 1) * BLOCK_SIZE,
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

        // Game state logic
        #pragma region
        if (player.life == 0) {
            game.playing = 0;
            break;
        }
        else if (game.reset) {
            game.reset = 0;
            game.cam_moving = 0;
            game.cam_row = 0;
            game.cam_pos_x = 0;
            // Reset character state (except life!)
            player.shield                       = 0;
            player.lightstaff                   = 0;
            player.forward                      = 1;
            player.power_used                   = 0;
            player.spell_radius                 = 0;
            player.timer                        = 0;
            player.coins                        = 0;
            player.physics.pos_x                = game.map->start_row * BLOCK_SIZE;
            player.physics.pos_y                = (NUM_BLOCKS_Y - game.map->start_column - 1) * BLOCK_SIZE;
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
            spawn_enemies(game.map, game.cam_pos_x, game.cam_row, game.cam_row + NUM_BLOCKS_X);
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

        // Compute player
        #pragma region
        check_player_state(&game, &player, gamepad_poll_button(&button_C));
        update_player_position(&game, &player, gamepad_read_joystick_axis(adc_handle, &joystick.axis_x));
        if (check_block_collisions(game.map, &player.physics, game.cam_row)) {
            apply_reactive_force(&player.physics);
        }
        // Update platform position & check for collision with the player
        for (uint8_t i = 0; i < MAX_PLATFORMS; i++) {
            update_platform_position(&game, &platforms[i]);
            if (check_platform_collision(&player.physics, &platforms[i])) {
                player.physics.platform_i = i; // FIXME: is it still worth using platform_i?
                player.physics.pos_y -= player.physics.speed_y; // "reactive force"
                // If moving, follow the movement of the platform
                if (platforms[i].moved && platforms[i].horizontal) {
                    player.physics.pos_x += platforms[i].physics.speed_x;
                }
                else if (platforms[i].moved && platforms[i].vertical) {
                    player.physics.pos_y += platforms[i].physics.speed_y;
                }
            }
        }
        // Check for lightstaff usage
        if (player.lightstaff && gamepad_poll_button(&button_A)) {
            player.power_used = 1;
        }
        #pragma endregion

        // Compute interactive blocks that have been hit by the player
        for (uint8_t i = 0; i < NUM_BLOCK_RECORDS; i++) {
            if (!blocks[i].is_hit || blocks[i].row == -1 || blocks[i].column == -1) {
                continue;
            }
            compute_interactive_block(&game, &blocks[i]);
        }

        // Compute items
        for (uint8_t i = 0; i < NUM_ITEMS; i++) {
            if (is_player_collecting_item(&game, &player, &items[i])) {
                collect_item(&player, &items[i]);
            }
        }

        // Spawn & compute enemies
        spawn_enemies(game.map, game.cam_pos_x, SPAWN_START(game.cam_row), SPAWN_END(game.cam_row) + 1);
        for (int i = 0; i < NUM_ENEMY_RECORDS; i++) {
            compute_enemy(&game, &player, &enemies[i]);
        }

        // Compute projectiles
        for (uint8_t i = 0; i < MAX_PROJECTILES; i++) {
            compute_projectile(&game, &player, &projectiles[i]);
        }

        // Frame construction
        build_frame(&game, &player);
        st7735s_draw_text(&coins_text_object);
        st7735s_draw_text(&life_text_object);
        sprintf(num_coins_text, "%i", (uint8_t)game.coins + player.coins);
        sprintf(num_life_text, "%i", (uint8_t)player.life);
        st7735s_draw_text(&coins_object);
        st7735s_draw_text(&life_object);

        // Send the frame to the display
        st7735s_push_frame(tft_handle);

        // Feed the task watchdog timer
        TIMERG0.wdtwprotect.wdt_wkey = TIMG_WDT_WKEY_VALUE;
        TIMERG0.wdtfeed.wdt_feed = 1;
        TIMERG0.wdtwprotect.wdt_wkey = 0;
    }
}