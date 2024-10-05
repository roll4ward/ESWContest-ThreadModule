#include "bluetooth_ad.h"
#include "ble_uuid.h"
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/logging/log.h>
#include <hw_id.h>

#define DEVICE_NAME_BUFFER_SIZE        50U
#define HW_ID_DISPLAY_LEN              6U

LOG_MODULE_REGISTER(bluetooth_ad, LOG_LEVEL_DBG);

static struct bt_le_adv_param *adv_param = BT_LE_ADV_PARAM(
	(BT_LE_ADV_OPT_CONNECTABLE |
	 BT_LE_ADV_OPT_USE_IDENTITY), /* Connectable advertising and use identity address */
	0x0500, // 500 ms
	0x0600, // 800 ms
	NULL); /* Set to NULL for undirected advertising */



static struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR | BT_LE_AD_GENERAL),
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_COMMISSION_SERVICE_VAL),
};

static char device_name[DEVICE_NAME_BUFFER_SIZE] = "Roll4 Network Module";

static struct bt_data sd[] = {
    {.type = BT_DATA_NAME_COMPLETE,
     .data = device_name}
};

static void set_device_name_adv();

int start_bt_advertise() {
    if(!bt_is_ready()) {
        LOG_INF("Bluetooth is not ready. Enable Bluetooth");
        int err = bt_enable(NULL);
        if (err < 0) {
            return err;
        }
        LOG_INF("Bluetooth Enabled");
    }
    set_device_name_adv();

    return bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));   
}

static void set_device_name_adv() {
    char hw_id[HW_ID_LEN];
    int res;

    res = hw_id_get(hw_id, HW_ID_LEN);
    if (res < 0) {
        LOG_ERR("Failed to Get HW ID : %d", res);
        return;
    }

    strcat(device_name, "-");
    strncat(device_name, hw_id, HW_ID_DISPLAY_LEN);

    sd[0].data_len = strlen(device_name);

    LOG_DBG("ID : %s", hw_id);
}

int stop_bt_advertise() {
    return bt_le_adv_stop();
}