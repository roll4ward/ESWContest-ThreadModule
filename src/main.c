#include <zephyr/kernel.h>
#include <zephyr/net/openthread.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#include "bluetooth_ad.h"
#include "bluetooth_conn.h"
#include "action_button.h"
#include "button_service.h"

int main(void)
{
        start_bt_advertise();
        return 0;
}