#include "st7735s.h"

/**
 * TODO: 16/04/2023
 * Think about which structs to make (pixels, sprites, ect), take the color thing in mind
 * Make orientation functions
 * Have another look at the datasheet and make any other useful functions
 * Optimize refresh rate (instead of updating each pixel with a for loop, make sure an entire frame can be passed through send_data() and do it)
 * SPI data transfer 64 bytes max
 */


void init_spi(void)
{
    const spi_bus_config_t spi_bus_cfg = {
        .mosi_io_num = PIN_LCD_SDA,
        .miso_io_num = -1,
        .sclk_io_num = PIN_LCD_SCK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1
    };
    spi_bus_initialize(SPI_LCD_HOST, &spi_bus_cfg, SPI_LCD_DMA);

    // Create SPI device handle
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
}


void send_command(uint8_t command)
{
    spi_transaction_t transaction;
    memset(&transaction, 0, sizeof(transaction));
    transaction.length = 8;
    transaction.tx_buffer = &command;
    // Enable command mode
    gpio_set_level(PIN_LCD_DC, 0);
    ESP_ERROR_CHECK(spi_device_polling_transmit(tft_handle, &transaction));
}


void send_data(uint8_t *data, int len)
{
    if (len > MAX_TRANSFER_SIZE)
    {
        printf("Error: Data limit reached for SPI transaction.\n    \
                -----> Max: %i\t bytes\n                             \
                -----> Cur: %i\t bytes\n", MAX_TRANSFER_SIZE, len);
        return;
    }
    spi_transaction_t transaction;
    memset(&transaction, 0, sizeof(transaction));
    transaction.length = 8 * len;
    transaction.tx_buffer = data;
    // Enable data mode
    gpio_set_level(PIN_LCD_DC, 1); 
    ESP_ERROR_CHECK(spi_device_polling_transmit(tft_handle, &transaction));
}


void init_tft(void)
{
    uint8_t parameter;
    // Hardware reset
    gpio_set_level(PIN_LCD_RES, 0);
    ets_delay_us(10);
    gpio_set_level(PIN_LCD_RES, 1);
    ets_delay_us(120*1000);

    // Reset software
    send_command(SWRESET);
    ets_delay_us(120*1000);

    // Sleep out
    send_command(SLPOUT);
    ets_delay_us(250*1000);

    // Pixel format 16-bit
    send_command(COLMOD);
    parameter = LCD_COLOR_FORMAT;
    send_data(&parameter, sizeof(parameter));

    // Frame rate control
    send_command(FRMCTR1);
    uint8_t param_FRMCTRL1[3] = {LCD_RTNA, LCD_FPA, LCD_BPA};
    send_data(param_FRMCTRL1, sizeof(param_FRMCTRL1));

    // Memory data access control
    send_command(MADCTL);
    parameter = ((LCD_MY << 8) | (LCD_MX << 7) | (LCD_MV << 6) |
                (LCD_ML << 5) | (LCD_RGB << 4) | (LCD_MH << 3));
    send_data(&parameter, sizeof(parameter));

    // Display inversion off
    send_command(INVOFF);

    // Power control 1 - Default applied
    send_command(PWCTR1);
    uint8_t param_PWCTR1[3] = {0xA8, 0x08, 0x84};
    send_data(param_PWCTR1, sizeof(param_PWCTR1));

    // Power control 2 - Default applied
    send_command(PWCTR2);
    parameter = 0xC0;
    send_data(&parameter, sizeof(parameter));

    // VCOM control - Default applied
    send_command(VMCTR1);
    parameter = 0x05;
    send_data(&parameter, sizeof(parameter));

    // Display function control
    send_command(INVCTR);
    parameter = 0x00;
    send_data(&parameter, sizeof(parameter));

    // Gamma curve
    send_command(GAMSET);
    parameter = LCD_GAMMA; 
    send_data(&parameter, sizeof(parameter));

    // Normal display mode ON
    send_command(NORON);
    
    // Switch display on
    send_command(DISPON);
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


void set_backlight(uint8_t percent)
{
    uint32_t duty = (uint32_t) (pow(2, PWM_LCD_RESOLUTION) - 1) * percent / 100;
    // Set duty cycle
    ledc_set_duty(PWM_LCD_MODE, PWM_LCD_CHANNEL, duty);
    // Update duty to apply the new value
    ledc_update_duty(PWM_LCD_MODE, PWM_LCD_CHANNEL);
}


void set_display_area(uint8_t xs, uint8_t xe, uint8_t ys, uint8_t ye)
{
    // Set the column
    send_command(CASET);
    uint8_t columns[4] = {0x00, xs, 0x00, xe};
    send_data(columns, sizeof(columns));

    // Set the row
    send_command(RASET);
    uint8_t rows[4] = {0x00, ys, 0x00, ye};
    send_data(rows, sizeof(rows));
}


void create_frame(void)
{
    
}



int check_frame_size(int len)
{
    uint8_t bits_per_pixel;
    switch (LCD_COLOR_FORMAT)
    {
        case 0x03: // Adress for 12-bit per pixel
            bits_per_pixel = 12;
            break;
        case 0x05: // Adress for 16-bit per pixel
            bits_per_pixel = 16;
            break;
        case 0x06: // Adress for 18-bit per pixel
            bits_per_pixel = 18;
            break;
        default:
            printf("Error: COLMOD (0x3A) not set\n");
            return 1;
    }
    int num_max_bytes = ceil((float)LCD_HEIGHT * LCD_WIDTH * bits_per_pixel / 8);
    if (len > num_max_bytes)
    {
        printf("Warning: Frame size exceeds ST7735S memory size.\
                Data will be lost.\n");
        return 1;
    }
    return 0;
}


void push_frame(uint16_t *frame, int len)
{
    check_frame_size(len);
    /**
     * Number of transaction required to send the frame to the display.
     * As a reminder:
     * - With DMA disabled, spi_bus_cfg.max_transfer_sz = 64 bytes
     * - With DMA enabled, spi_bus_cfg.max_transfer_sz = 4092 bytes
     */
    uint16_t num_transactions = ceil((float)len / MAX_TRANSFER_SIZE);
    /**
     * Fill a data array with the amount of pixels the array can
     * carry: (MAX_TRANSFER_SIZE / 2) pixels for 16-bit color format.
     * Hence, the frame is chopped in (MAX_TRANSFER_SIZE / 2)-pixel 
     * packets, each packet being sent one at the time.
     */
    uint8_t data[MAX_TRANSFER_SIZE];
    uint16_t px_per_trans = MAX_TRANSFER_SIZE / 2;
    send_command(RAMWR);
    for (int i = 0; i < num_transactions; i++)
    {
        for (int j = 0; j < px_per_trans; j++)
        {
            data[2*j] = frame[i*px_per_trans+j] >> 8;
            data[2*j+1] = frame[i*px_per_trans+j] & 0xFF;
        }
        send_data(data, MAX_TRANSFER_SIZE);
    }
}