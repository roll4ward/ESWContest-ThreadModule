#include "bluetooth_ad.h"
#include "zephyr/bluetooth/bluetooth.h"
#include "zephyr/bluetooth/gap.h"
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bluetooth_ad, LOG_LEVEL_INF);

static struct bt_le_adv_param *adv_param = BT_LE_ADV_PARAM(
	(BT_LE_ADV_OPT_CONNECTABLE |
	 BT_LE_ADV_OPT_USE_IDENTITY), /* Connectable advertising and use identity address */
	BT_GAP_ADV_FAST_INT_MIN_1, /* 0x30 units, 48 units, 30ms */
	BT_GAP_ADV_FAST_INT_MAX_1, /* 0x60 units, 96 units, 60ms */
	NULL); /* Set to NULL for undirected advertising */


static struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR | BT_LE_AD_GENERAL),
    BT_DATA(BT_DATA_NAME_COMPLETE, "Roll4ward", 9),
};

static struct bt_data sd[] = {
    BT_DATA(BT_DATA_NAME_SHORTENED, "Roll4", 5)
};

int start_bt_advertise() {
    if(!bt_is_ready()) {
        LOG_INF("Bluetooth is not ready. Enable Bluetooth");
        int err = bt_enable(NULL);
        if (err < 0) {
            return err;
        }
        LOG_INF("Bluetooth Enabled");
    }

    return bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));   
}