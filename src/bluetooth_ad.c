#include "bluetooth_ad.h"
#include "zephyr/bluetooth/bluetooth.h"
#include "zephyr/bluetooth/gap.h"
#ifdef CONFIG_LOG
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bluetooth_ad, LOG_LEVEL_INF);
#endif

static struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR | BT_LE_AD_GENERAL),
    BT_DATA(BT_DATA_NAME_COMPLETE, "Roll4ward", 9),
};

static struct bt_data sd[] = {
    BT_DATA(BT_DATA_NAME_SHORTENED, "Roll4", 5)
};

static struct bt_le_adv_param *adv_param;

int start_bt_advertise() {
    if(!bt_is_ready()) {
        LOG_INF("Bluetooth is not ready. Enable Bluetooth");
        int err = bt_enable(NULL);
        if (err < 0) {
            return err;
        }
    }

    return bt_le_adv_start(BT_LE_ADV_NCONN, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));   
}