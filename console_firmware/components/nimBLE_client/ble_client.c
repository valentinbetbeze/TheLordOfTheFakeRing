#include "ble_client.h"

int8_t ble_axis_X;
button_t ble_button_A, ble_button_C;

static const char *TAG = "NimBLE_BLE_CENT";
static const char CONTROLLER_NAME[] = "ESP32_CONTROLLER";

#define NUM_CHARACTERISTICS 3

// 59462f12-9543-9999-12c8-58b459a2712d
static const ble_uuid128_t gatt_svr_svc_gamepad_uuid =
    BLE_UUID128_INIT(0x2d, 0x71, 0xa2, 0x59, 0xb4, 0x58, 0xc8, 0x12,
                     0x99, 0x99, 0x43, 0x95, 0x12, 0x2f, 0x46, 0x59);

// 5c3a659e-897e-45e1-b016-007107c96df6
static const ble_uuid128_t gatt_svr_chr_axis_x_uuid =
    BLE_UUID128_INIT(0xf6, 0x6d, 0xc9, 0x07, 0x71, 0x00, 0x16, 0xb0,
                     0xe1, 0x45, 0x7e, 0x89, 0x9e, 0x65, 0x3a, 0x5c);

// 5c3a659e-897e-45e1-b016-007107c96df7
static const ble_uuid128_t gatt_svr_chr_button_A_uuid =
    BLE_UUID128_INIT(0xf7, 0x6d, 0xc9, 0x07, 0x71, 0x00, 0x16, 0xb0,
                     0xe1, 0x45, 0x7e, 0x89, 0x9e, 0x65, 0x3a, 0x5c);

// 5792bd31-c8db-493a-80a8-3a1da2c9f48f
static const ble_uuid128_t gatt_svr_chr_button_C_uuid =
    BLE_UUID128_INIT(0x8f, 0xf4, 0xc9, 0xa2, 0x1d, 0x3a, 0xa8, 0x80,
                     0x3a, 0x49, 0xdb, 0xc8, 0x31, 0xbd, 0x92, 0x57);

/**
 * Characteristic handles storage
 * 
 * chr_handles[0] = ble_axis_x handle
 * chr_handles[1] = ble_button_A handle
 * chr_handles[2] = ble_button_C handle
 */
static uint16_t chr_handles[NUM_CHARACTERISTICS];
static uint16_t myconn_handle;

static int blecent_gap_event(struct ble_gap_event *event, void *arg);
void ble_store_config_init(void);


static uint16_t blecent_check_characteristic(const struct peer *peer,
                                             const ble_uuid_t *svc_uuid,
                                             const ble_uuid_t *chr_uuid)
{
    const struct peer_chr *chr;
    int rc;

    // Find characteristic
    chr = peer_chr_find_uuid(peer, svc_uuid, chr_uuid);
    if (chr == NULL) {
        char *buf = calloc(BLE_UUID_TYPE_128, sizeof(char));
        buf = ble_uuid_to_str(chr_uuid, buf);
        MODLOG_DFLT(ERROR, "Error: Peer doesn't support characteristic: %s", buf);
        free(buf);
        ble_gap_terminate(peer->conn_handle, BLE_ERR_REM_USER_CONN_TERM);
        return 0;
    }

    // Read characteristic
    rc = ble_gattc_read(peer->conn_handle, chr->chr.val_handle, NULL, NULL);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "Error: Failed to read characteristic; rc=%d\n", rc);
        ble_gap_terminate(peer->conn_handle, BLE_ERR_REM_USER_CONN_TERM);
        return 0;
    }

    return chr->chr.val_handle;
}


/**
 * @brief Retrieve all characteristic handles and load them in memory.
 */
static void blecent_get_gatt_handles(const struct peer *peer)
{
    // Check axis_x characteristic
    chr_handles[0] = blecent_check_characteristic(peer,
                                                  &gatt_svr_svc_gamepad_uuid.u,
                                                  &gatt_svr_chr_axis_x_uuid.u);
    // Check button_A characteristic
    chr_handles[1] = blecent_check_characteristic(peer,
                                                  &gatt_svr_svc_gamepad_uuid.u,
                                                  &gatt_svr_chr_button_A_uuid.u);
    // Check button_C characteristic
    chr_handles[2] = blecent_check_characteristic(peer,
                                                  &gatt_svr_svc_gamepad_uuid.u,
                                                  &gatt_svr_chr_button_C_uuid.u);
}


/**
 * Called when service discovery of the specified peer has completed.
 */
static void blecent_on_disc_complete(const struct peer *peer, int status, 
                                     void *arg)
{
    if (status != 0) {
        /* Service discovery failed.  Terminate the connection. */
        MODLOG_DFLT(ERROR, "Error: Service discovery failed; status=%d "
                    "conn_handle=%d\n", status, peer->conn_handle);
        ble_gap_terminate(peer->conn_handle, BLE_ERR_REM_USER_CONN_TERM);
        return;
    }

    /* Service discovery has completed successfully. Now we have a complete
     * list of services, characteristics, and descriptors that the peer
     * supports.
     */
    MODLOG_DFLT(INFO, "Service discovery complete; status=%d "
                "conn_handle=%d\n", status, peer->conn_handle);

    // Check all services and characteristics
    blecent_get_gatt_handles(peer);
}


/**
 * Initiates the GAP general discovery procedure.
 */
static void blecent_scan(void)
{
    uint8_t own_addr_type;
    struct ble_gap_disc_params disc_params;
    int rc;

    /* Figure out address to use while advertising (no privacy for now) */
    rc = ble_hs_id_infer_auto(0, &own_addr_type);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "error determining address type; rc=%d\n", rc);
        return;
    }

    /* Tell the controller to filter duplicates; we don't want to process
     * repeated advertisements from the same device.
     */
    disc_params.filter_duplicates = 1;

    /* Set scan parameters */
    disc_params.passive = 0;
    disc_params.itvl = 0;
    disc_params.window = 0;
    disc_params.filter_policy = 0;
    disc_params.limited = 0;

    rc = ble_gap_disc(own_addr_type, BLE_HS_FOREVER, &disc_params,
                      blecent_gap_event, NULL);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "Error initiating GAP discovery procedure; rc=%d\n", rc);
    }
}


/**
 * Indicates whether we should try to connect to the sender of the specified
 * advertisement. The function returns a positive result if the device
 * advertises connectability and has the game controller's name.
 */
static int blecent_should_connect(const struct ble_gap_disc_desc *disc)
{
    struct ble_hs_adv_fields fields;
    int rc;

    /* The device has to be advertising connectability. */
    if (disc->event_type != BLE_HCI_ADV_RPT_EVTYPE_ADV_IND &&
            disc->event_type != BLE_HCI_ADV_RPT_EVTYPE_DIR_IND) {
        return 0;
    }

    rc = ble_hs_adv_parse_fields(&fields, disc->data, disc->length_data);
    if (rc != 0) {
        return rc;
    }
    
    // Check device name
    char name[BLE_HS_ADV_MAX_SZ];
    if (fields.name != NULL) {
        assert(fields.name_len < sizeof(name) - 1);
        memcpy(name, fields.name, fields.name_len);
        name[fields.name_len] = '\0';
        if (!strncmp(name, CONTROLLER_NAME, BLE_HS_ADV_MAX_SZ)) {
            return 1;
        }
    }
    
    return 0;
}


/**
 * Connects to the sender of the specified advertisement of it looks
 * interesting.  A device is "interesting" if it advertises connectability and
 * is the controller.
 */
static void blecent_connect_if_interesting(void *disc)
{
    uint8_t own_addr_type;
    int rc;
    ble_addr_t *addr;

    /* Don't do anything if we don't care about this advertiser. */
    if (!blecent_should_connect((struct ble_gap_disc_desc *)disc)) {
        return;
    }

    /* Scanning must be stopped before a connection can be initiated. */
    rc = ble_gap_disc_cancel();
    if (rc != 0) {
        MODLOG_DFLT(DEBUG, "Failed to cancel scan; rc=%d\n", rc);
        return;
    }

    /* Figure out address to use for connect (no privacy for now) */
    rc = ble_hs_id_infer_auto(0, &own_addr_type);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "error determining address type; rc=%d\n", rc);
        return;
    }

    /* Try to connect the the advertiser.  Allow 30 seconds (30000 ms) for
     * timeout.
     */
    addr = &((struct ble_gap_disc_desc *)disc)->addr;

    rc = ble_gap_connect(own_addr_type, addr, 30000, NULL,
                         blecent_gap_event, NULL);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "Error: Failed to connect to device; addr_type=%d "
                    "addr=%s; rc=%d\n",
                    addr->type, addr_str(addr->val), rc);
        return;
    }
}


/**
 * The nimble host executes this callback when a GAP event occurs.  The
 * application associates a GAP event callback with each connection that is
 * established.  blecent uses the same callback for all connections.
 *
 * @param event                 The event being signalled.
 * @param arg                   Application-specified argument; unused by
 *                                  blecent.
 *
 * @return                      0 if the application successfully handled the
 *                                  event; nonzero on failure.  The semantics
 *                                  of the return code is specific to the
 *                                  particular GAP event being signalled.
 */
static int blecent_gap_event(struct ble_gap_event *event, void *arg)
{
    struct ble_gap_conn_desc desc;
    struct ble_hs_adv_fields fields;
    memset(&fields, 0, sizeof(fields));
    int rc;

    switch (event->type) {
        case BLE_GAP_EVENT_DISC:
            rc = ble_hs_adv_parse_fields(&fields, event->disc.data,
                                        event->disc.length_data);
            if (rc != 0) {
                return 0;
            }
            /* An advertisement report was received during GAP discovery. */
            char s[BLE_HS_ADV_MAX_SZ];
            if (fields.name != NULL) {
                assert(fields.name_len < sizeof s - 1);
                memcpy(s, fields.name, fields.name_len);
                s[fields.name_len] = '\0';
                MODLOG_DFLT(INFO, "Device detected: %s%s", s,
                            fields.name_is_complete ? "" : "...");
            }
            else {
                MODLOG_DFLT(WARN, "Unknown device detected");
            }

            /* Try to connect to the advertiser if it looks interesting. */
            blecent_connect_if_interesting(&event->disc);

            return 0;

        case BLE_GAP_EVENT_CONNECT:
            /* A new connection was established or a connection attempt failed. */
            if (event->connect.status == 0) {
                /* Connection successfully established. Save handle
                 for later use */
                myconn_handle = event->connect.conn_handle;
                MODLOG_DFLT(INFO, "Connection established ");

                rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
                assert(rc == 0);
                print_conn_desc(&desc);
                MODLOG_DFLT(INFO, "\n");

                /* Remember peer. */
                rc = peer_add(event->connect.conn_handle);
                if (rc != 0) {
                    MODLOG_DFLT(ERROR, "Failed to add peer; rc=%d\n", rc);
                    return 0;
                }

                /* Perform service discovery. */
                rc = peer_disc_all(event->connect.conn_handle,
                                blecent_on_disc_complete, NULL);
                if (rc != 0) {
                    MODLOG_DFLT(ERROR, "Failed to discover services; rc=%d\n", rc);
                    return 0;
                }
            }
            else {
                /* Connection attempt failed; resume scanning. */
                MODLOG_DFLT(ERROR, "Error: Connection failed; status=%d\n",
                            event->connect.status);
                blecent_scan();
            }

            return 0;

        case BLE_GAP_EVENT_DISCONNECT:
            /* Connection terminated. */
            MODLOG_DFLT(INFO, "disconnect; reason=%d ", event->disconnect.reason);
            print_conn_desc(&event->disconnect.conn);
            MODLOG_DFLT(INFO, "\n");

            /* Forget about peer. */
            peer_delete(event->disconnect.conn.conn_handle);

            /* Resume scanning. */
            blecent_scan();
            return 0;

        case BLE_GAP_EVENT_DISC_COMPLETE:
            MODLOG_DFLT(INFO, "discovery complete; reason=%d\n",
                        event->disc_complete.reason);
            return 0;

        default:
            return 0;
    }
}


static void blecent_on_reset(int reason)
{
    MODLOG_DFLT(ERROR, "Resetting state; reason=%d\n", reason);
}


static void blecent_on_sync(void)
{
    int rc;

    /* Make sure we have proper identity address set (public preferred) */
    rc = ble_hs_util_ensure_addr(0);
    assert(rc == 0);

    /* Begin scanning for a peripheral to connect to. */
    blecent_scan();
}


static void blecent_host_task(void *param)
{
    ESP_LOGI(TAG, "BLE Host Task Started");
    /* This function will return only when nimble_port_stop() is executed */
    nimble_port_run();

    nimble_port_freertos_deinit();
}


static int read_axis_X(uint16_t conn_handle,
                       const struct ble_gatt_error *error,
                       struct ble_gatt_attr *attr, void *arg)
{
    if (error->status == 0) {
        uint8_t temp = 0;
        for (int i = 0; i < attr->om->om_len; i++) {
            temp |= attr->om->om_data[i] << (8 * i);
        }
        ble_axis_X = temp;
    }
    else {
        MODLOG_DFLT(ERROR, "Error: Failed to read ble_axis_X");
    }
    return 0;
}


static int read_button_A(uint16_t conn_handle,
                         const struct ble_gatt_error *error,
                         struct ble_gatt_attr *attr, void *arg)
{
    if (error->status == 0) {
        ble_button_A.current_state = *(attr->om->om_data);
    }
    else {
        MODLOG_DFLT(ERROR, "Error: Failed to read ble_button_A");
    }
    return 0;
}


static int read_button_C(uint16_t conn_handle,
                         const struct ble_gatt_error *error,
                         struct ble_gatt_attr *attr, void *arg)
{
    if (error->status == 0) {
        ble_button_C.current_state = *(attr->om->om_data);
    }
    else {
        MODLOG_DFLT(ERROR, "Error: Failed to read ble_button_A");
    }
    return 0;
}


void nimBLE_client_initialize_ble(void)
{
    int rc;
    // Initialize NVS — it is used to store PHY calibration data
    esp_err_t ret = nvs_flash_init();
    if  (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    nimble_port_init();

    // Configure the host.
    ble_hs_cfg.reset_cb = blecent_on_reset;
    ble_hs_cfg.sync_cb = blecent_on_sync;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

    // Initialize data structures to track connected peers
    rc = peer_init(MYNEWT_VAL(BLE_MAX_CONNECTIONS), 64, 64, 64);
    assert(rc == 0);

    // Set the default device name
    rc = ble_svc_gap_device_name_set("ESP32-CONSOLE");
    assert(rc == 0);

    /* XXX Need to have template for store */
    ble_store_config_init();

    nimble_port_freertos_init(blecent_host_task);
}


void nimBLE_client_read_gamepad(void)
{
    ble_gattc_read(myconn_handle, chr_handles[0], read_axis_X, NULL);

    ble_gattc_read(myconn_handle, chr_handles[1], read_button_A, NULL);
    filter_push_signal(&ble_button_A);

    ble_gattc_read(myconn_handle, chr_handles[2], read_button_C, NULL);
    filter_push_signal(&ble_button_C);
}
