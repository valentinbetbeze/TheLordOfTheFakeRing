#ifndef __MHFMD_DRIVER_H__
#define __MHFMD_DRIVER_H__

#include <math.h>
#include <assert.h>

#include "driver/ledc.h"
#include "driver/gpio.h"

/**
 * @brief Initialize the buzzer's PWM signal.
 */
void mhfmd_init_pwm(void);

/**
 * @brief Set the buzzer on or off.
 * 
 * @param state 1 for the buzzer to be on, else 0.
 */
void mhfmd_set_buzzer(const uint8_t state);

/**
 * @brief Set the frequency of the buzzer.
 * 
 * @param freq Frequency of the buzzer, in Hz.
 */
void mhfmd_set_frequency(const uint16_t freq);


#endif // __MHFMD_DRIVER_H__