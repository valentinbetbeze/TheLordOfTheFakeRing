#include "joystick.h"

void init_button(button_t *btn, gpio_num_t gpio_num)
{
    btn->gpio_num = gpio_num;
    gpio_reset_pin(btn->gpio_num);
    gpio_set_direction(btn->gpio_num, GPIO_MODE_INPUT);
    gpio_set_pull_mode(btn->gpio_num, GPIO_PULLUP_ONLY);
    gpio_set_intr_type(btn->gpio_num, GPIO_INTR_DISABLE);
}

uint8_t poll_button(button_t *btn)
{
    btn->current_state = (uint8_t) gpio_get_level(btn->gpio_num);
    if (btn->current_state != btn->previous_state)
    {
        // Update previous state
        btn->previous_state = btn->current_state;
        // If the current state is low (e.g. pushed down)
        if (!btn->current_state) return 1;
    }
    return 0;
}

joystick_t create_joystick(uint16_t idle_val_x, uint16_t idle_val_y)
{
    joystick_t joystick;
    const uint8_t y_scale = 100;        /*!< So that JOY_MAX_N = 100%                   */

    // x-axis configuration
    joystick.axis_x.idle_value = idle_val_x;
    joystick.axis_x.l_slope = (float) y_scale / idle_val_x;
    joystick.axis_x.r_slope = (float) y_scale / (JOY_MAX_N - idle_val_x);
    joystick.axis_x.l_offset = (int8_t) -1 * y_scale;
    joystick.axis_x.r_offset = (int8_t) -1 * joystick.axis_x.r_slope * idle_val_x;

    // y-axis configuration
    joystick.axis_y.idle_value = idle_val_y;
    joystick.axis_y.l_slope = (float) y_scale / idle_val_y;
    joystick.axis_y.r_slope = (float) y_scale / (JOY_MAX_N - idle_val_y);
    joystick.axis_y.l_offset = (int8_t) -1 * y_scale;
    joystick.axis_y.r_offset = (int8_t) -1 * joystick.axis_y.r_slope * idle_val_y;

    return joystick;
}

void init_joystick(adc_oneshot_unit_handle_t *handle, joystick_t *joystick)
{
    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id = ADC_UNIT_2,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    
    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = ADC_ATTEN_DB_11,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };

    // Configure ADC driver
    adc_oneshot_new_unit(&unit_cfg, handle);

    // Set axis channels
    adc_oneshot_io_to_channel(PIN_JOY_X, &(unit_cfg.unit_id), &(joystick->axis_x.channel));
    adc_oneshot_io_to_channel(PIN_JOY_Y, &(unit_cfg.unit_id), &(joystick->axis_y.channel));

    // Configure ADC channels
    adc_oneshot_config_channel(*handle, joystick->axis_x.channel, &chan_cfg);
    adc_oneshot_config_channel(*handle, joystick->axis_y.channel, &chan_cfg);
}

int8_t read_joystick_axis(adc_oneshot_unit_handle_t handle, axis_t axis)
{
    int8_t value = 0;
    int raw_value = 0;
    adc_oneshot_read(handle, axis.channel, &raw_value);

    // Left side of the resistive track
    if (raw_value < axis.idle_value)
    {
        value = axis.l_slope * raw_value + axis.l_offset;
    }
    // Right side of the resistive track
    else if (raw_value > axis.idle_value)
    {
        value = axis.r_slope * raw_value + axis.r_offset;
    }
    else value = 0;

    return value;
}
