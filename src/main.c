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

int main(void)
{
        int err;
        uint32_t events;
        extern struct action_button bt_button;

        init_button_service();
        register_button(&bt_button);

        LOG_INF("button initialize finished. : %d", err);
        while (1){
                events = k_event_wait(&(bt_button.press_event), 
                                        BT_BUTTON_SHORT_PRESS | BT_BUTTON_LONG_PRESS,
                                        true, K_FOREVER);

                LOG_INF("Event Occured");

                switch (events) {
                        case BT_BUTTON_SHORT_PRESS:
                                LOG_INF("Button Short pressed.");
                                break;
                        case BT_BUTTON_LONG_PRESS:
                                LOG_INF("Button Long pressed.");
                                break;
                }
        }
        
        return 0;
}