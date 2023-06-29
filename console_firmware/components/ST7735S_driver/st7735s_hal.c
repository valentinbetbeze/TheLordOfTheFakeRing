#include "include/st7735s_hal.h"

uint16_t frame[NUM_TRANSACTIONS][PX_PER_TRANSACTION] = {0};


/**
 * @brief Send a command to the ST7735S chip.
 * 
 * @param[in] handle SPI device handle of the display.
 * @param[in] command 8-bit command (see ST7735S datasheet p.104)
 */
static void send_command(const spi_device_handle_t handle, uint8_t const command)
{
    spi_transaction_t transaction;
    memset(&transaction, 0, sizeof(transaction));
    transaction.length = 8;
    transaction.tx_buffer = &command;
    
    ESP_ERROR_CHECK(gpio_set_level(PIN_LCD_DC, 0)); // Enable command mode
    ESP_ERROR_CHECK(spi_device_polling_transmit(handle, &transaction));
}


/**
 * @brief Send a packet of 8-bit data to the ST7735S chip.
 * 
 * @param[in] handle SPI device handle of the display.
 * @param[in] data Pointer to the 8-bit data to be sent.
 * @param[in] len Amount of data in byte.
 * 
 * @note This function is useful to send command arguments to the ST7735S
 * driver, or 8-bit color format data. It is recommended to make use of the
 * send_words() function if sending 16-bit color format data.
 */
static void send_bytes(const spi_device_handle_t handle, const uint8_t *data,
                       const uint16_t len)
{
    spi_transaction_t transaction;
    memset(&transaction, 0, sizeof(transaction));
    transaction.length = 8 * len;
    transaction.tx_buffer = (void *)data;
    
    gpio_set_level(PIN_LCD_DC, 1); // Enable data mode
    ESP_ERROR_CHECK(spi_device_polling_transmit(handle, &transaction));
}


/**
 * @brief Send a packet of 16-bit data to the ST7735S chip.
 * 
 * @param[in] handle SPI device handle of the display.
 * @param[in] data Pointer to the 16-bit data to be sent.
 * @param[in] len Amount of data in byte.
 * 
 * @note send_words() shall be used instead of send_bytes() when 16-bit data,
 * typically 16-bit color format, has to be sent. Making a single function for
 * both 8-bit and 16-bit data input would require processing the size of the data
 * and its length before initiating the SPI transaction. This would add unnecessary
 * load onto the processor, hence the use of the two (rather similar) functions.
 */
static void send_words(const spi_device_handle_t handle, const uint16_t *data,
                       const uint16_t len)
{
    spi_transaction_t transaction;
    memset(&transaction, 0, sizeof(transaction));
    transaction.length = 8 * len;
    transaction.tx_buffer = (void *)data;
    
    gpio_set_level(PIN_LCD_DC, 1); // Enable data mode
    ESP_ERROR_CHECK(spi_device_polling_transmit(handle, &transaction));
}


void st7735s_init_pwm_backlight(void)
{
    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t LCD_timer_config = {
        .speed_mode             = PWM_LCD_MODE,
        .duty_resolution        = PWM_LCD_RESOLUTION,
        .timer_num              = PWM_LCD_TIMER,
        .freq_hz                = PWM_LCD_FREQ,
        .clk_cfg                = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&LCD_timer_config));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t LCD_timer_channel = {
        .gpio_num               = PIN_LCD_BKL,
        .speed_mode             = PWM_LCD_MODE,
        .channel                = PWM_LCD_CHANNEL,
        .intr_type              = LEDC_INTR_DISABLE,
        .timer_sel              = PWM_LCD_TIMER,
        .duty                   = 0,
        .hpoint                 = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&LCD_timer_channel));
}


void st7735s_set_backlight(uint8_t percentage)
{
    if (percentage > 100) {
        percentage = 100;
    }
    uint32_t duty = (uint32_t) (pow(2, PWM_LCD_RESOLUTION) - 1) * percentage / 100;
    // Set duty cycle
    ESP_ERROR_CHECK(ledc_set_duty(PWM_LCD_MODE, PWM_LCD_CHANNEL, duty));
    // Update duty to apply the new value
    ESP_ERROR_CHECK(ledc_update_duty(PWM_LCD_MODE, PWM_LCD_CHANNEL));
}


void st7735s_init_spi(spi_device_handle_t *handle)
{
    if (handle == NULL) {
        printf("Error(st7735s_init_spi): handle pointer is NULL.\n");
        assert(handle);
    }
    // Initialize non-SPI GPIOs
    const gpio_config_t io_conf = {
        .pin_bit_mask = ((1 << PIN_LCD_DC) | (1 << PIN_LCD_RES)),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));
    // Configure SPI bus
    const spi_bus_config_t spi_bus_cfg = {
        .mosi_io_num = PIN_LCD_SDA,
        .miso_io_num = -1,
        .sclk_io_num = PIN_LCD_SCK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI_LCD_HOST, &spi_bus_cfg, SPI_LCD_DMA));
    // Add the TFT display as SPI device
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
    ESP_ERROR_CHECK(spi_bus_add_device(SPI_LCD_HOST, &spi_dev_cfg, handle));
}


void st7735s_init_tft(const spi_device_handle_t handle)
{
    uint8_t parameter;
    // Hardware reset
    ESP_ERROR_CHECK(gpio_set_level(PIN_LCD_RES, 0));
    ets_delay_us(10);
    ESP_ERROR_CHECK(gpio_set_level(PIN_LCD_RES, 1));
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
    send_bytes(handle, &parameter, sizeof(parameter));

    // Frame rate control
    send_command(handle, FRMCTR1);
    uint8_t param_FRMCTRL1[3] = {LCD_RTNA, LCD_FPA, LCD_BPA};
    send_bytes(handle, param_FRMCTRL1, sizeof(param_FRMCTRL1));

    // Memory data access control
    send_command(handle, MADCTL);
    parameter = ((LCD_MY << 7) | (LCD_MX << 6) | (LCD_MV << 5) |
                (LCD_ML << 4) | (LCD_RGB << 3) | (LCD_MH << 2));
    send_bytes(handle, &parameter, sizeof(parameter));

    // Display inversion off
    send_command(handle, INVOFF);

    // Power control 1 - Default applied
    send_command(handle, PWCTR1);
    uint8_t param_PWCTR1[3] = {0xA8, 0x08, 0x84};
    send_bytes(handle, param_PWCTR1, sizeof(param_PWCTR1));

    // Power control 2 - Default applied
    send_command(handle, PWCTR2);
    parameter = 0xC0;
    send_bytes(handle, &parameter, sizeof(parameter));

    // VCOM control - Default applied
    send_command(handle, VMCTR1);
    parameter = 0x05;
    send_bytes(handle, &parameter, sizeof(parameter));

    // Display function control
    send_command(handle, INVCTR);
    parameter = 0x00;
    send_bytes(handle, &parameter, sizeof(parameter));

    // Gamma curve
    send_command(handle, GAMSET);
    parameter = LCD_GAMMA; 
    send_bytes(handle, &parameter, sizeof(parameter));

    // Normal display mode ON
    send_command(handle, NORON);
    
    // Switch display on
    send_command(handle, DISPON);

    // Set all columns
    send_command(handle, CASET);
    uint8_t columns[4] = {0x00, 0x00, 0x00, 0x7F};
    send_bytes(handle, columns, sizeof(columns));

    // Set all rows
    send_command(handle, RASET);
    uint8_t rows[4] = {0x00, 0x00, 0x00, 0x9F};
    send_bytes(handle, rows, sizeof(rows));
}


void st7735s_push_frame(const spi_device_handle_t handle)
{
    send_command(handle, RAMWR);
    // Push the frame to the ST7735S LCD driver.
    for (uint16_t i = 0; i < NUM_TRANSACTIONS; i++) {
        if (REMAINING_PIXELS && (i == NUM_TRANSACTIONS - 1)) {
            // Extract the last pixels to avoid sending garbage values
            uint16_t last_pixels[REMAINING_PIXELS] = {0};
            for (uint16_t j = 0; j < REMAINING_PIXELS; j++) {
                last_pixels[j] = frame[i][j];
            }
            send_words(handle, last_pixels, REMAINING_PIXELS * sizeof(uint16_t));
        }
        else {
            send_words(handle, frame[i], MAX_TRANSFER_SIZE);
        }
    }
}
