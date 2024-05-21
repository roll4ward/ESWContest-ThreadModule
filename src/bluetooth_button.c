#include "action_button.h"
#include "bluetooth_ad.h"
#include "bluetooth_conn.h"

#define     BT_BUTTON_NODE                           DT_ALIAS(bluetooth_button)

static enum bt_status {
        waiting,
        advertise,
        connected,
};

enum bt_status status = waiting;

static void do_advertise();
static void do_waiting();
static void do_unconnect();

static void bt_on_long(struct k_work *work) {
        LOG_INF("bluetooth LONG PRESSED");
        switch(status) {
                case waiting:
                do_advertise();
                break;

                case advertise:
                do_waiting();
                break;

                case connected:
                do_unconnect();
                break;
        }
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