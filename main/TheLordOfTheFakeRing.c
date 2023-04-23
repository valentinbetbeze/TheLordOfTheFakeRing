/**
 * @file TheLordOfTheFakeRing.c
 * @author valentin betbeze (valentin.betbeze@gmail.com)
 * @brief Main code of the game.
 * @date 2023-04-16
 * 
 * @note
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "project_config.h"
#include "gamepad.h"
#include "st7735s_hal.h"
#include "st7735s_graphics.h"
#include "interface.h"
#include "rom/ets_sys.h"


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
    gpio_config(&io_conf);

    // Initialize SPI bus
    const spi_bus_config_t spi_bus_cfg = {
        .mosi_io_num = PIN_LCD_SDA,
        .miso_io_num = -1,
        .sclk_io_num = PIN_LCD_SCK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1
    };
    spi_bus_initialize(SPI_LCD_HOST, &spi_bus_cfg, SPI_LCD_DMA);

    // Create SPI device handle
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
    spi_bus_add_device(SPI_LCD_HOST, &spi_dev_cfg, &tft_handle);

    // Initialize LCD display
    st7735s_init_tft(tft_handle);
    st7735s_fill_background(BLACK);
    st7735s_init_pwm_backlight();
    st7735s_set_backlight(50);
#pragma endregion


    /*************************************************
     * Program body
     *************************************************/
    uint8_t nitems = 0;
    item_t items[MAX_ITEMS] = {0};      /* Array of graphic items */

    while(1) {
        //usleep(10*1000);
        for (int i = 0; i < 16; i++) {
            nitems = scan_map(shire, i, items);
            st7735s_build_frame(items, nitems);
            st7735s_push_frame(tft_handle);
            usleep(10*1000);
        }
        for (int i = 15; i >= 0; i--) {
            nitems = scan_map(shire, i, items);
            st7735s_build_frame(items, nitems);
            st7735s_push_frame(tft_handle);
            usleep(10*1000);
        }
    }
}




/* JOYSTICK MAIN CODE
void app_main(void)
{
    // Configure buttons
    button_t button_A, button_B, button_C, button_D, button_E, button_F;
    gamepad_init_button(&button_A, PIN_BTN_A);
    gamepad_init_button(&button_B, PIN_BTN_B);
    gamepad_init_button(&button_C, PIN_BTN_C);
    gamepad_init_button(&button_D, PIN_BTN_D);
    gamepad_init_button(&button_E, PIN_BTN_E);
    gamepad_init_button(&button_F, PIN_BTN_F);

    // Create and initialize joystick
    joystick_t joystick = create_joystick(JOY_MID_X, JOY_MID_Y);
    adc_oneshot_unit_handle_t adc_handle;
    gamepad_init_joystick(&adc_handle, &joystick);

    while (1) {
        ets_delay_us(10*1000);

        // Poll buttons
        if (gamepad_poll_button(&button_A)) printf("Button A pushed!\n");
        if (gamepad_poll_button(&button_B)) printf("Button B pushed!\n");
        if (gamepad_poll_button(&button_C)) printf("Button C pushed!\n");
        if (gamepad_poll_button(&button_D)) printf("Button D pushed!\n");
        if (gamepad_poll_button(&button_E)) printf("Button E pushed!\n");
        if (gamepad_poll_button(&button_F)) printf("Button F pushed!\n");

        // Read joystick values
        int8_t x_pos = gamepad_read_joystick_axis(adc_handle, joystick.axis_x);
        int8_t y_pos = gamepad_read_joystick_axis(adc_handle, joystick.axis_y);

        printf("xPos: %i\t", x_pos);
        printf("yPos: %i\n", y_pos);
    }
}
*/