#include <zephyr/kernel.h>
#include <zephyr/net/openthread.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#include "thread_net_config.h"
#include "bluetooth_ad.h"
#include "bluetooth_conn.h"
#include "button.h"

int main(void)
{
        int err;
        uint32_t events;
        extern struct k_event bt_button_press_event;

        err = init_button_service();
        if (err < 0) {
                LOG_ERR("Button init failed. : %d", err);
                return -1;
        }

        LOG_INF("button initialize finished. : %d", err);
        while (1){
                events = k_event_wait(&bt_button_press_event, 
                                        BT_BUTTON_SHORT_PRESS | BT_BUTTON_LONG_PRESS,
                                        true, K_FOREVER);

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