#include "gamepad.h"

void gamepad_init_button(button_t *btn, const gpio_num_t gpio_num)
{
    if (btn == NULL) {
        printf("Error(gamepad_init_button): button_t object does not exist.\n");
        assert(btn);
    }
    btn->gpio_num = gpio_num;
    ESP_ERROR_CHECK(gpio_reset_pin(btn->gpio_num));
    ESP_ERROR_CHECK(gpio_set_direction(btn->gpio_num, GPIO_MODE_INPUT));
    ESP_ERROR_CHECK(gpio_set_pull_mode(btn->gpio_num, GPIO_PULLUP_ONLY));
    ESP_ERROR_CHECK(gpio_set_intr_type(btn->gpio_num, GPIO_INTR_DISABLE));
}


void filter_push_signal(button_t *btn)
{
    if (btn == NULL) {
        printf("Error(filter_push_signal): button_t object does not exist.\n");
        assert(btn);
    }
    if (btn->current_state != btn->previous_state) {
        // Update previous state
        btn->previous_state = btn->current_state;
        // If the current state is low (e.g. pushed down)
        if (!btn->current_state) {
            btn->pushed = 1;
            return;
        }
    }
    btn->pushed = 0;
}


void gamepad_poll_button(button_t *btn)
{
    if (btn == NULL) {
        printf("Error(gamepad_poll_button): button_t object does not exist.\n");
        assert(btn);
    }
    btn->current_state = (uint8_t) gpio_get_level(btn->gpio_num);
    filter_push_signal(btn);
}


void gamepad_config_joystick(joystick_t *joystick, const uint16_t idle_val_x, const uint16_t idle_val_y)
{
    if (joystick == NULL) {
        printf("Error(gamepad_config_joystick): joystick_t object does not exist.\n");
        assert(joystick);
    }
    const uint8_t y_scale = 100; // So that JOY_MAX_N = 100%
    // x-axis configuration
    joystick->axis_x.idle_value = idle_val_x;
    joystick->axis_x.l_slope = (float) y_scale / idle_val_x;
    joystick->axis_x.r_slope = (float) y_scale / (JOY_MAX_N - idle_val_x);
    joystick->axis_x.l_offset = (int8_t) -1 * y_scale;
    joystick->axis_x.r_offset = (int8_t) -1 * joystick->axis_x.r_slope * idle_val_x;

    // y-axis configuration
    joystick->axis_y.idle_value = idle_val_y;
    joystick->axis_y.l_slope = (float) y_scale / idle_val_y;
    joystick->axis_y.r_slope = (float) y_scale / (JOY_MAX_N - idle_val_y);
    joystick->axis_y.l_offset = (int8_t) -1 * y_scale;
    joystick->axis_y.r_offset = (int8_t) -1 * joystick->axis_y.r_slope * idle_val_y;
}


void gamepad_init_joystick(adc_oneshot_unit_handle_t *handle, joystick_t *joystick)
{
    if (handle == NULL) {
        printf("Error(gamepad_init_joystick): object `handle` does not exist.\n");
        assert(handle);
    }
    if (joystick == NULL) {
        printf("Error(gamepad_init_joystick): joystick_t object does not exist.\n");
        assert(joystick);
    }
    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id = ADC_UNIT_2,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = ADC_ATTEN_DB_11,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    // Configure ADC driver
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, handle));
    // Set axis channels
    ESP_ERROR_CHECK(adc_oneshot_io_to_channel(PIN_JOY_X, &(unit_cfg.unit_id), &(joystick->axis_x.channel)));
    // Configure ADC channels
    ESP_ERROR_CHECK(adc_oneshot_config_channel(*handle, joystick->axis_x.channel, &chan_cfg));
}


int8_t gamepad_read_joystick_axis(const adc_oneshot_unit_handle_t handle, axis_t *axis)
{
    if (axis == NULL) {
        printf("Error(gamepad_read_joystick_axis): axis_t object does not exist.\n");
        assert(axis);
    }
    int8_t value = 0;
    int raw_value = 0;
    ESP_ERROR_CHECK(adc_oneshot_read(handle, axis->channel, &raw_value));

    // Left side of the resistive track
    if (raw_value < axis->idle_value) {
        value = axis->l_slope * raw_value + axis->l_offset;
    }
    // Right side of the resistive track
    else if (raw_value > axis->idle_value) {
        value = axis->r_slope * raw_value + axis->r_offset;
    }
    else value = 0;

    return value;
}
