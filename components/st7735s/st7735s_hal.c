#include "st7735s_hal.h"

uint16_t frame[NUM_TRANSACTIONS][PX_PER_TRANSACTION] = {0};


/**
 * @brief Send a command to the ST7735S chip.
 * 
 * @param handle SPI device handle of the display.
 * @param command 8-bit command (see ST7735S datasheet p.104)
 */
static void send_command(spi_device_handle_t handle, uint8_t command)
{
    spi_transaction_t transaction;
    memset(&transaction, 0, sizeof(transaction));
    transaction.length = 8;
    transaction.tx_buffer = &command;
    
    gpio_set_level(PIN_LCD_DC, 0); // Enable command mode
    ESP_ERROR_CHECK(spi_device_polling_transmit(handle, &transaction));
}


/**
 * @brief Send a byte to the ST7735S chip. // TODO: rename & description
 * 
 * @param handle SPI device handle of the display.
 * @param data Pointer to the data to be sent.
 * @param len Amount of data in byte.
 */
static void send_byte(spi_device_handle_t handle, uint8_t *data, size_t len)
{
    spi_transaction_t transaction;
    memset(&transaction, 0, sizeof(transaction));
    transaction.length = 8 * len;
    transaction.tx_buffer = data;
    
    gpio_set_level(PIN_LCD_DC, 1); // Enable data mode
    ESP_ERROR_CHECK(spi_device_polling_transmit(handle, &transaction));
}


/**
 * @brief Send a WORD (2 bytes) to the ST7735S chip.
 * 
 * @param handle SPI device handle of the display.
 * @param data Pointer to the data to be sent.
 * @param len Amount of data in byte.
 * 
 * @details //TODO: explain why send_byte & send_word (why 2 functions and not 1)
 */
static void send_word(spi_device_handle_t handle, uint16_t *data, size_t len)
{
    spi_transaction_t transaction;
    memset(&transaction, 0, sizeof(transaction));
    transaction.length = 8 * len;
    transaction.tx_buffer = (uint16_t *)data;
    
    gpio_set_level(PIN_LCD_DC, 1); // Enable data mode
    ESP_ERROR_CHECK(spi_device_polling_transmit(handle, &transaction));
}


void st7735s_init_pwm_backlight(void)
{
    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t LCD_timer_config = {
        .speed_mode             = PWM_LCD_MODE,
        .duty_resolution        = PWM_LCD_RESOLUTION,
        .timer_num              = PWM_LCD_CHANNEL,
        .freq_hz                = PWM_LCD_FREQ,
        .clk_cfg                = LEDC_AUTO_CLK
    };
    ledc_timer_config(&LCD_timer_config);

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t LCD_timer_channel = {
        .gpio_num               = PIN_LCD_BKL,
        .speed_mode             = PWM_LCD_MODE,
        .channel                = PWM_LCD_CHANNEL,
        .intr_type              = LEDC_INTR_DISABLE,
        .timer_sel              = PWM_LCD_GROUP,
        .duty                   = 0,
        .hpoint                 = 0
    };
    ledc_channel_config(&LCD_timer_channel);
}


void st7735s_set_backlight(uint8_t percent)
{
    if (percent > 100) {
        percent = 100;
    }
    uint32_t duty = (uint32_t) (pow(2, PWM_LCD_RESOLUTION) - 1) * percent / 100;
    // Set duty cycle
    ledc_set_duty(PWM_LCD_MODE, PWM_LCD_CHANNEL, duty);
    // Update duty to apply the new value
    ledc_update_duty(PWM_LCD_MODE, PWM_LCD_CHANNEL);
}


void st7735s_init_tft(spi_device_handle_t handle)
{
    uint8_t parameter;
    // Hardware reset
    gpio_set_level(PIN_LCD_RES, 0);
    ets_delay_us(10);
    gpio_set_level(PIN_LCD_RES, 1);
    ets_delay_us(120*1000);

    // Reset software
    send_command(handle, SWRESET);
    ets_delay_us(120*1000);

    // Sleep out
    send_command(handle, SLPOUT);
    ets_delay_us(250*1000);

    // Pixel format 16-bit
    send_command(handle, COLMOD);
    parameter = LCD_COLOR_FORMAT;
    send_byte(handle, &parameter, sizeof(parameter));

    // Frame rate control
    send_command(handle, FRMCTR1);
    uint8_t param_FRMCTRL1[3] = {LCD_RTNA, LCD_FPA, LCD_BPA};
    send_byte(handle, param_FRMCTRL1, sizeof(param_FRMCTRL1));

    // Memory data access control
    send_command(handle, MADCTL);
    parameter = ((LCD_MY << 7) | (LCD_MX << 6) | (LCD_MV << 5) |
                (LCD_ML << 4) | (LCD_RGB << 3) | (LCD_MH << 2));
    send_byte(handle, &parameter, sizeof(parameter));

    // Display inversion off
    send_command(handle, INVOFF);

    // Power control 1 - Default applied
    send_command(handle, PWCTR1);
    uint8_t param_PWCTR1[3] = {0xA8, 0x08, 0x84};
    send_byte(handle, param_PWCTR1, sizeof(param_PWCTR1));

    // Power control 2 - Default applied
    send_command(handle, PWCTR2);
    parameter = 0xC0;
    send_byte(handle, &parameter, sizeof(parameter));

    // VCOM control - Default applied
    send_command(handle, VMCTR1);
    parameter = 0x05;
    send_byte(handle, &parameter, sizeof(parameter));

    // Display function control
    send_command(handle, INVCTR);
    parameter = 0x00;
    send_byte(handle, &parameter, sizeof(parameter));

    // Gamma curve
    send_command(handle, GAMSET);
    parameter = LCD_GAMMA; 
    send_byte(handle, &parameter, sizeof(parameter));

    // Normal display mode ON
    send_command(handle, NORON);
    
    // Switch display on
    send_command(handle, DISPON);

    // Set all columns
    send_command(handle, CASET);
    uint8_t columns[4] = {0x00, 0x00, 0x00, 0x7F};
    send_byte(handle, columns, sizeof(columns));

    // Set all rows
    send_command(handle, RASET);
    uint8_t rows[4] = {0x00, 0x00, 0x00, 0x9F};
    send_byte(handle, rows, sizeof(rows));
}


void st7735s_push_frame(spi_device_handle_t handle)
{
    send_command(handle, RAMWR);
    for (int i = 0; i < NUM_TRANSACTIONS; i++) {
        send_word(handle, frame[i], MAX_TRANSFER_SIZE);
    }
}
