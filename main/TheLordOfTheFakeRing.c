/**
 * @file TheLordOfTheFakeRing.c
 * @author valentin betbeze (valentin.betbeze@gmail.com)
 * @brief Main code of the game.
 * @version 0.1
 * @date 2023-04-16
 * 
 * @note
 */

#include <stdio.h>
#include <unistd.h>

#include "rom/ets_sys.h"
#include "project_config.h"
#include "joystick.h"
#include "st7735s.h"
#include "graphics.h"



spi_device_handle_t tft_handle;


void app_main()
{
    // Initialize SPI bus
    init_spi();

    // Set display backlight
    init_pwm_backlight();
    set_backlight(30);
    
    // Initialize non-SPI GPIOs
    const gpio_config_t io_conf = {
        .pin_bit_mask = ((1 << PIN_LCD_DC) | (1 << PIN_LCD_RES)),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE
    };
    gpio_config(&io_conf);

    // Initialize LCD display
    init_tft();

    // Fill screen with unicolor
    uint16_t color = hex_RGB888_to_RGB565(0x0FF0000);
    set_display_area(0, 127, 0, 159);
    send_command(RAMWR);
    for (int i = 0; i < (128*160); i++)
    {           
        send_word(&color, sizeof(color));
    }

    while(1)
    {
        sleep(1);
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

    while (1)
    {
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