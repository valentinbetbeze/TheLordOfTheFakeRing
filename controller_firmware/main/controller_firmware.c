/**
 * @author valentin betbeze (valentin.betbeze@gmail.com)
 * @brief Source code of the game's controller.
 * @date 2023-06-19
 */

#include "nvs_flash.h"
#include "gamepad.h"
#include "ble_server.h"

joystick_t joystick;
adc_oneshot_unit_handle_t adc_handle;
button_t button_A, button_C;

const char *CONTROLLER_NAME = "ESP32_CONTROLLER";


void app_main(void)
{
    int rc;

    /* Initialize NVS — it is used to store PHY calibration data */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize host and controller stack
    nimble_port_init();

    // Initialize NimBLE host configuration parameters and callbacks
    nimble_host_config_init();

    rc = nimble_gatt_svr_init();
    assert(rc == 0);

    /* Set the default device name. */
    rc = ble_svc_gap_device_name_set(CONTROLLER_NAME);
    assert(rc == 0);

    /* XXX Need to have template for store */
    ble_store_config_init();

    nimble_port_freertos_init(nimble_bleprph_host_task);

    // Initialize joystick
    gamepad_config_joystick(&joystick, JOY_MID_X, JOY_MID_Y);
    gamepad_init_joystick(&adc_handle, &joystick);

    // Initialize buttons
    gamepad_init_button(&button_A, PIN_BTN_A);
    gamepad_init_button(&button_C, PIN_BTN_C);
}
