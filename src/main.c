#include <zephyr/kernel.h>
#include <zephyr/net/openthread.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#include "bluetooth_ad.h"
#include "ble_commissioning.h"
#include "coap.h"
#include "util.h"
#include "action_button.h"
#include "button_service.h"
#include "reset_network.h"

static void update_value(UserData *data) {
        *((int *)(data->mUserData)) += 1;
        LOG_INF("Value Updated");
}

DEFINE_COAP_USER_DATA(int, user_data, update_value);
DEFINE_COAP_RESOURCE(value, &user_data);


int main(void)
{
        openthread_start(openthread_get_default_context());
        init_ble_commission();
        init_button_service();
        init_reset_network();

        start_bt_advertise();
        
        LOG_INF("START: add CoAP Resource");
        addCoAPResource(&value_resource);
        LOG_INF("END: add CoAP Resource");

        LOG_INF("START: CoAP Start");
        EXPECT_NO_ERROR_OR_DO(otCoapStart(openthread_get_default_instance(), 6000), LOG_ERR("Failed to Start CoAP"));
        LOG_INF("END: CoAP Start");

        return 0;
}