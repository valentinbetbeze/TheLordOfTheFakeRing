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
#include "joystick.h"
#include "st7735s_hal.h"
#include "st7735s_graphics.h"
#include "rom/ets_sys.h"

/**
 * @brief TFT display handle on the SPI bus
 * @note Set as global variable to avoid having to pass it as an 
 * arguments every time an SPI communication takes place.
 */
spi_device_handle_t tft_handle;

/**
 * @brief 
 * 
 */
uint16_t frame[NUM_TRANSACTIONS][PX_PER_TRANSACTION];


void app_main()
{
    // Initialize non-SPI GPIOs
    const gpio_config_t io_conf = {
        .pin_bit_mask = ((1 << PIN_LCD_DC) | (1 << PIN_LCD_RES)),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE
    };
    gpio_config(&io_conf);

    // Initialize SPI bus
    init_spi();

    // Initialize LCD display
    init_tft();
    set_display_area(0, 127, 0, 159);

    // Set background
    fill_background(BLACK);

    // Set display backlight
    init_pwm_backlight();
    set_backlight(30);
    
    uint8_t string1[] = "THE LORD OF\nTHE FAKE RING\nAWAITS...";
    text_t text1 = {
        .color      = GREEN,
        .data       = string1,
        .size       = sizeof(string1),
        .pos_x      = 10,
        .pos_y      = 80,
    };
    //draw_text(text1);

    circle_t circle = {
        .color      = WHITE,
        .pos_x      = LCD_WIDTH / 4,
        .pos_y      = LCD_HEIGHT / 2,
        .radius     = 30,
        .thickness  = 1,
    };
    draw_circle(circle);
    circle_t circle2 = {
        .color      = SPI_SWAP_DATA_TX(0xFFDF, 16),
        .pos_x      = 3 * LCD_WIDTH / 4,
        .pos_y      = LCD_HEIGHT / 2,
        .radius     = 30,
        .thickness  = 5,
    };
    draw_circle(circle2);

    push_frame();

    while(1) {
        usleep(10*1000);
    }
}




/* JOYSTICK MAIN CODE
void app_main(void)
{
    // Configure buttons
    button_t button_A, button_B, button_C, button_D, button_E, button_F;
    init_button(&button_A, PIN_BTN_A);
    init_button(&button_B, PIN_BTN_B);
    init_button(&button_C, PIN_BTN_C);
    init_button(&button_D, PIN_BTN_D);
    init_button(&button_E, PIN_BTN_E);
    init_button(&button_F, PIN_BTN_F);

    // Create and initialize joystick
    joystick_t joystick = create_joystick(JOY_MID_X, JOY_MID_Y);
    adc_oneshot_unit_handle_t adc_handle;
    init_joystick(&adc_handle, &joystick);

    while (1) {
        ets_delay_us(10*1000);

        // Poll buttons
        if (poll_button(&button_A)) printf("Button A pushed!\n");
        if (poll_button(&button_B)) printf("Button B pushed!\n");
        if (poll_button(&button_C)) printf("Button C pushed!\n");
        if (poll_button(&button_D)) printf("Button D pushed!\n");
        if (poll_button(&button_E)) printf("Button E pushed!\n");
        if (poll_button(&button_F)) printf("Button F pushed!\n");

        // Read joystick values
        int8_t x_pos = read_joystick_axis(adc_handle, joystick.axis_x);
        int8_t y_pos = read_joystick_axis(adc_handle, joystick.axis_y);

        printf("xPos: %i\t", x_pos);
        printf("yPos: %i\n", y_pos);
    }
}
*/