#include "MH-FMD_driver.h"

#define PIN_BUZZER      21


void mhfmd_init_pwm(void)
{   
    ledc_timer_config_t buz_timer_config = {
        .speed_mode             = LEDC_LOW_SPEED_MODE,
        .duty_resolution        = LEDC_TIMER_5_BIT,
        .timer_num              = LEDC_TIMER_0,
        .freq_hz                = 100,
        .clk_cfg                = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&buz_timer_config));

    ledc_channel_config_t buz_timer_channel = {
        .gpio_num               = PIN_BUZZER,
        .speed_mode             = LEDC_LOW_SPEED_MODE,
        .channel                = LEDC_CHANNEL_0,
        .intr_type              = LEDC_INTR_DISABLE,
        .timer_sel              = LEDC_TIMER_0,
        .duty                   = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&buz_timer_channel));
}


void mhfmd_set_buzzer(const uint8_t state)
{
    uint32_t duty;
    if (state) {
        duty = (pow(2, LEDC_TIMER_5_BIT) - 1) / 2;
    }
    else {
        duty = 0;
    }
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
}


void mhfmd_set_frequency(const uint16_t freq)
{
    ESP_ERROR_CHECK(ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0, freq));
}
