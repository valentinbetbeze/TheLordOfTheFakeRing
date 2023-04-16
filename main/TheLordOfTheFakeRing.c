#include <stdio.h>

#include "joystick.h"
#include "st7735s.h"
#include "rom/ets_sys.h"

spi_device_handle_t tft_handle;


/**
 * TODO: 16/04/2023
 * Think about which structs to make (pixels, sprites, ect), take the color thing in mind
 * Make orientation functions
 * Have another look at the datasheet and make any other useful functions
 * Optimize refresh rate (instead of updating each pixel with a for loop, make sure an entire frame can be passed through send_data() and do it)
 */
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
    //uint16_t color = RGB888_to_RGB565(255, 255, 255);
    uint16_t color = hex_RGB888_to_RGB565(0x0000FF);
    uint8_t data[2] = {color >> 8, color & 0xFF};
    set_write_area(0, 0, 127, 159);
    for (int i = 0; i < (128 * 160); i++)
    {
        send_data(data, sizeof(data));
    }

    while(1)
    {
        ets_delay_us(100*1000);
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