#include <zephyr/kernel.h>
#include <zephyr/net/openthread.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#include "thread_net_config.h"
#include "bluetooth_ad.h"
#include "bluetooth_conn.h"
#include "action_button.h"
#include "button_service.h"

#define     DEVICE_BUTTON_NODE                       DT_ALIAS(device_button)

void dv_on_long(struct k_work *work) {
        LOG_INF("device LONG PRESSED");
}

void dv_on_short(struct k_work *work) {
        LOG_INF("device SHORT PRESSED");
}


struct action_button_callback dv_callback = {
        .on_long_press = dv_on_long,
        .on_short_press = dv_on_short,
};

struct action_button dev_button = {
    .dt_spec= GPIO_DT_SPEC_GET(DEVICE_BUTTON_NODE, gpios),
    .press_status = released,
};

extern struct action_button            bt_button;
extern struct action_button_callback   bt_callback;

int main(void)
{
        init_button_service();
        register_button(&bt_button, &bt_callback);
        register_button(&dev_button, &dv_callback);
        
        return 0;
}