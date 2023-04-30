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

    const int8_t (*map)[NB_BLOCKS_Y] = shire;

    uint16_t map_x = 0;
    character_t player = {
        .life               = 3,
        .speed_x            = INITIAL_SPEED,
        .speed_y            = INITIAL_SPEED,
        .sprite.height      = BLOCK_SIZE,
        .sprite.width       = BLOCK_SIZE,
        .sprite.data        = sprite_player,
        .sprite.pos_x       = 16,
        .sprite.pos_y       = 0,
    };

    while(1) {
        // Feed the task watchdog timer
        TIMERG0.wdtwprotect.wdt_wkey = TIMG_WDT_WKEY_VALUE;
        TIMERG0.wdtfeed.wdt_feed = 1;
        TIMERG0.wdtwprotect.wdt_wkey = 0;

        // Get the time for the current iteration
        uint64_t time;
        ESP_ERROR_CHECK(gptimer_get_raw_count(timer_handle, &time));
        time = (uint64_t)time / 10; // Convert to milliseconds
        
        // Update the player's x-position
        int8_t x = gamepad_read_joystick_axis(adc_handle, joystick.axis_x);
        player.sprite.pos_x += player.speed_x * (int16_t)(x / 100);
        if (player.sprite.pos_x + BLOCK_SIZE / 2 > LCD_WIDTH / 2) {
            map_x += player.speed_x;
            player.sprite.pos_x = LCD_WIDTH / 2 - BLOCK_SIZE / 2;
        }
        else if (player.sprite.pos_x < 0) {
            player.sprite.pos_x = 0;
        }

        // Check if starting or ending a jump
        if ((!player.jumping && gamepad_poll_button(&button_C)) ||
            (!player.jumping && !player.bottom_collision)) {

            player.t0 = (uint32_t)time;
            player.jumping = 1;
        }
        else if (player.jumping && player.bottom_collision) {
            player.jumping = 0;
            player.speed_y = INITIAL_SPEED;
        }

        // Update the player's y-position
        player.accelerating = (uint8_t)(((uint32_t)time - player.t0) / DELTA_T);
        if (player.jumping && player.accelerating) {
            player.t0 = (uint32_t)time;
            player.accelerating = 0;
            player.speed_y += 1;
            printf("speed: %i\n", player.speed_y);
        }
        player.sprite.pos_y += player.speed_y;

        // Check for collisions with the environment, and update the player's position if required
        if (-1 == check_collisions(map, &player, map_x)) {
            break;
        }

        // Build and display the frame
        build_frame(map, map_x);
        st7735s_draw_sprite(player.sprite);
        st7735s_push_frame(tft_handle);
    }
}