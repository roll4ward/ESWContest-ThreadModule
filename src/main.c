#include <zephyr/kernel.h>
#include <zephyr/net/openthread.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#include "thread_net_config.h"
#include "bluetooth_ad.h"
#include "bluetooth_conn.h"

int main(void)
{
        int err;
        struct openthread_context *otContext = openthread_get_default_context();

        err = openthread_start(otContext);
        if (err < 0) {
                LOG_ERR("Openthread Start Error");
                return 0;
        }
        register_bt_conn_cb();
        err = start_bt_advertise();
        if(err < 0) {
                LOG_ERR("Start Bluetooth Advertise Error : %d", err);
                return 0;        
        }

        LOG_INF("Successfully started Bluetooth");

        return 0;
}