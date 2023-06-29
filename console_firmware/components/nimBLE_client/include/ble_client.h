#ifndef __BLE_CLIENT_H__
#define __BLE_CLIENT_H__

#include <stdio.h>
#include <assert.h>

#include "esp_log.h"
#include "nvs_flash.h"

#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "console/console.h"
#include "services/gap/ble_svc_gap.h"
#include "esp_central.h"

/**
 * @brief Store the previous and current states of a button
 * variable to allow filtering its desired value.
 */
typedef struct {
    uint8_t pre_state;  // previous state
    uint8_t cur_state;  // current state
    uint8_t value;      // post-processed value
} ble_button_t;

extern int8_t ble_axis_X;
extern ble_button_t ble_button_A;
extern ble_button_t ble_button_C;

/**
 * @brief Initialize the BLE client on the ESP32 central
 */
void nimBLE_client_initialize_ble(void);

/**
 * @brief Send a read request to the BLE server to get the state
 * of the buttons and joystick's axis.
 */
void nimBLE_client_read_gamepad(void);


#endif // __BLE_CLIENT_H__