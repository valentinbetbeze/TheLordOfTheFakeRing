/**
 * @file gamepad.h
 * @author valentin betbeze (valentin.betbeze@gmail.com)
 * @brief Header file of the Funduino JoyStick V1.A gamepad.
 * @date 2023-04-10
 * 
 */

#ifndef __GAMEPAD_H__
#define __GAMEPAD_H__


#include <stdint.h>

#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_timer.h"
#include "esp_err.h"


/*************************************************
 * GPIOs and Constants
 *************************************************/
#define PIN_BTN_A		GPIO_NUM_34
#define PIN_BTN_B       GPIO_NUM_5
#define PIN_BTN_C       GPIO_NUM_19
#define PIN_BTN_D       GPIO_NUM_21
#define PIN_BTN_E       GPIO_NUM_23
#define PIN_BTN_F       GPIO_NUM_22
#define PIN_JOY_X       GPIO_NUM_25
#define PIN_JOY_Y       GPIO_NUM_26

#define JOY_MIN_N       0               /*!< Minimum ADC raw value for all axes */
#define JOY_MID_X       1920            /*!< ADC raw value while x-axis is idle */
#define JOY_MID_Y       1887            /*!< ADC raw value while y-axis is idle */
#define JOY_MAX_N       4095            /*!< Maximum ADC raw value for all axes */


/*************************************************
 * Data structures
 *************************************************/

/**
 * @brief Button state flags
 */
typedef struct {
    gpio_num_t gpio_num;            /*!< GPIO number                            */
    uint8_t previous_state;         /*!< Button state before the last polling   */
    uint8_t current_state;          /*!< Button state after the last polling    */
    uint64_t time_last_depressed;   /*!< Unused for now                         */
} button_t;

/**
 * @brief Joystick's axis parameters
 * @note Because the potentiometer inside the joystick is not perfectly
 * calibrated at 50% of the maximum resistance, two different linear
 * behaviours must be modelled. Hence the left @p `l_` and right @p `r_` 
 * parameters, each representing one side of the resistive track.
 */
typedef struct {
    uint16_t idle_value;            /*!< ADC raw value while the axis is idle   */
    float l_slope;                  /*!< Linear function slope - left side      */
    float r_slope;                  /*!< Linear function slope - right side     */
    int8_t l_offset;                /*!< Linear function offset - left side     */
    int8_t r_offset;                /*!< Linear function offset - right side    */
    adc_channel_t channel;          /*!< ADC channel of the axis                */
} axis_t;

/**
 * @brief Joystick axis object
 */
typedef struct {
    axis_t axis_x;
    axis_t axis_y;
} joystick_t;


/*************************************************
 * Prototypes
 *************************************************/

/**
 * @brief Initialize a button
 * 
 * @param btn       Button to initialize
 * @param gpio_num  GPIO pin of the button
 */
void gamepad_init_button(button_t *btn, gpio_num_t gpio_num);

/**
 * @brief Check if a button has been pressed.
 * 
 * @param btn   Button to poll
 * @return 1 if pressed, 0 otherwise
 */
uint8_t gamepad_poll_button(button_t *btn);

/**
 * @brief Create and configure a joystick object
 * 
 * @param idle_val_x ADC raw value while x-axis is idle
 * @param idle_val_y ADC raw value while y-axis is idle
 * @return Joystick object
 */
joystick_t gamepad_create_joystick(uint16_t idle_val_x, uint16_t idle_val_y);

/**
 * @brief Initialize the joystick.
 * 
 * @param handle ADC unit handle for oneshot mode
 * @param joystick Joystick's constants
 */
void gamepad_init_joystick(adc_oneshot_unit_handle_t *handle, joystick_t *joystick);

/**
 * @brief Read and process the value of a joystick's axis.
 * 
 * @param handle ADC unit handle for oneshot mode
 * @param axis Joystick axis
 * @return The processed value of the axis, on a scale from -100% to +100%
 */
int8_t gamepad_read_joystick_axis(adc_oneshot_unit_handle_t handle, axis_t axis);


#endif // __GAMEPAD_H__