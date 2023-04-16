#include "st7735s.h"

uint16_t hex_RGB888_to_RGB565(uint32_t rgb888)
{
    uint8_t red = (uint8_t) (((rgb888 >> 16) * 31) / 255);
    uint8_t green = (uint8_t) (((rgb888 >> 8 & 0xFF) * 63) / 255);
    uint8_t blue = (uint8_t) (((rgb888 & 0xFF) * 31) / 255);
    uint16_t rgb565 = ((red << 11) & RED) | ((green << 5) & GREEN) | (blue & BLUE);
    return rgb565;
}

uint16_t RGB888_to_RGB565(uint8_t red, uint8_t green, uint8_t blue)
{
    red = (uint8_t) ((red * 31) / 255);
    green = (uint8_t) ((green * 63) / 255);
    blue = (uint8_t) ((blue * 31) / 255);
    uint16_t rgb565 = ((red << 11) & RED) | ((green << 5) & GREEN) | (blue & BLUE);
    return rgb565;
}

void set_write_area(uint8_t xs, uint8_t ys, uint8_t xe, uint8_t ye)
{
    // Set the column
    send_command(CASET);
    uint8_t columns[4] = {0x00, xs, 0x00, xe};
    send_data(columns, sizeof(columns));

    // Set the row
    send_command(RASET);
    uint8_t rows[4] = {0x00, ys, 0x00, ye};
    send_data(rows, sizeof(rows));

    // Write command
    send_command(RAMWR);
}

void init_tft(void)
{
    uint8_t parameter;

    // Hardware reset
    gpio_set_level(PIN_LCD_RES, LOW);
    ets_delay_us(10);
    gpio_set_level(PIN_LCD_RES, HIGH);
    ets_delay_us(120*1000);

    // Reset software
    send_command(SWRESET);
    ets_delay_us(120*1000);

    // Sleep out
    send_command(SLPOUT);
    ets_delay_us(250*1000);

    // Pixel format 16-bit
    send_command(COLMOD);
    parameter = 0x05;
    send_data(&parameter, sizeof(parameter));

    // Frame rate control
    send_command(FRMCTR1);
    uint8_t param_FRMCTRL1[3] = {0x00, 0x06, 0x03};
    send_data(param_FRMCTRL1, sizeof(param_FRMCTRL1));

    // Memory data access control
    send_command(MADCTL);
    parameter = 0x00;
    send_data(&parameter, sizeof(parameter));

    // Display inversion off
    send_command(INVOFF);

    // Power control 1
    send_command(PWCTR1);
    uint8_t param_PWCTR1[3] = {0xA8, 0x08, 0x84};
    send_data(param_PWCTR1, sizeof(param_PWCTR1));

    // Power control 2
    send_command(PWCTR2);
    parameter = 0xC0;
    send_data(&parameter, sizeof(parameter));

    // VCOM control
    send_command(VMCTR1);
    parameter = 0x05;
    send_data(&parameter, sizeof(parameter));

    // Display function control
    send_command(INVCTR);
    parameter = 0x00;
    send_data(&parameter, sizeof(parameter));

    // Gamma curve
    send_command(GAMSET);
    parameter = 0x01; // Gamma curve 0
    send_data(&parameter, sizeof(parameter));

    // Normal display mode ON
    send_command(NORON);
    
    // Switch display on
    send_command(DISPON);
}

void send_data(uint8_t *data, int len)
{
    spi_transaction_t package;
    memset(&package, 0, sizeof(package));
    package.length = 8 * len;
    package.tx_buffer = data;
    
    gpio_set_level(PIN_LCD_DC, HIGH); // Data mode ON
    ESP_ERROR_CHECK(spi_device_polling_transmit(tft_handle, &package));
}

void send_command(uint8_t command)
{
    spi_transaction_t package;
    memset(&package, 0, sizeof(package));
    package.length = 8;
    package.tx_buffer = &command;

    gpio_set_level(PIN_LCD_DC, LOW); // Command mode ON
    ESP_ERROR_CHECK(spi_device_polling_transmit(tft_handle, &package));
}

void init_spi(void)
{
    const spi_bus_config_t spi_bus_cfg = {
        .mosi_io_num = PIN_LCD_SDA,
        .miso_io_num = -1,
        .sclk_io_num = PIN_LCD_SCK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1
    };
    spi_bus_initialize(VSPI_HOST, &spi_bus_cfg, SPI_DMA_DISABLED);

    // Create SPI device handle
    const spi_device_interface_config_t spi_dev_cfg = {
        .clock_speed_hz = SPI_MASTER_FREQ_10M,
        .mode = 0,                            
        .spics_io_num = PIN_LCD_CS, 
        .queue_size = 1,
        .flags = SPI_DEVICE_3WIRE | SPI_DEVICE_HALFDUPLEX,
        .command_bits = 0,
        .address_bits = 0,
        .dummy_bits = 0
    };
    spi_bus_add_device(VSPI_HOST, &spi_dev_cfg, &tft_handle);
}

void set_backlight(uint8_t percent)
{
    uint32_t duty = (uint32_t) (pow(2, PWM_LCD_RESOLUTION) - 1) * percent / 100;
    // Set duty cycle
    ledc_set_duty(PWM_LCD_MODE, PWM_LCD_CHANNEL, duty);
    // Update duty to apply the new value
    ledc_update_duty(PWM_LCD_MODE, PWM_LCD_CHANNEL);
}

void init_pwm_backlight(void)
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