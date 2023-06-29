#ifndef __BLE_SERVER_H__
#define __BLE_SERVER_H__

#include <stdbool.h>

#include "nimble/ble.h"
#include "modlog/modlog.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "services/gap/ble_svc_gap.h"

// server.c

void ble_store_config_init(void);

/**
 * @brief Initialize GAP and GATT server
 * 
 */
int nimble_gatt_svr_init(void);

/**
 * @brief Initialize the BLE host configuration
 * 
 */
void nimble_host_config_init(void);

/**
 * @brief Setup the BLE Host task
 * 
 */
void nimble_bleprph_host_task(void *param);


// utils.c
void print_bytes(const uint8_t *bytes, int len);
void print_addr(const void *addr);


#endif // __BLE_SERVER_H__