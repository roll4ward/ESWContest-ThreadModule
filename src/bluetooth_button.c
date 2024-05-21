#include "action_button.h"
#include "bluetooth_ad.h"
#include "bluetooth_conn.h"

#define     BT_BUTTON_NODE                           DT_ALIAS(bluetooth_button)

static void bt_on_long(struct k_work *work) {
        LOG_INF("bluetooth LONG PRESSED");
}

static  bt_on_short(struct k_work *work) {
        LOG_INF("bluetooth SHORT PRESSED");
}


struct action_button bt_button = {
    .dt_spec= GPIO_DT_SPEC_GET(BT_BUTTON_NODE, gpios),
    .press_status = released,
};

struct action_button_callback bt_callback = {
        .on_long_press = bt_on_long,
        .on_short_press = bt_on_short,
};