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

#define  BUTTON_NODE   DT_ALIAS(sw0)

DEFINE_ACTION_BUTTON(x, BUTTON_NODE, 5000);

BUTTON_CALLBACK(dv_on_long) {
        LOG_INF("device LONG PRESSED");
}

BUTTON_CALLBACK(dv_on_short) {
        LOG_INF("device SHORT PRESSED");
}

struct action_button_callback dv_callback = {
        .on_long_press = dv_on_long,
        .on_short_press = dv_on_short,
};

static void update_value(UserData *data) {
        *((int *)(data->mUserData)) += 1;
        LOG_INF("Value Updated");
}

DEFINE_COAP_USER_DATA(int, user_data, update_value);

DEFINE_COAP_RESOURCE(value, &user_data);

int main(void)
{
        openthread_start(openthread_get_default_context());
        start_bt_advertise();
        init_ble_commission();

        init_button_service();
        register_button(&x, &dv_callback);
        
        LOG_INF("START: add CoAP Resource");
        addCoAPResource(&value_resource);
        LOG_INF("END: add CoAP Resource");

        LOG_INF("START: CoAP Start");
        EXPECT_NO_ERROR_OR_DO(otCoapStart(openthread_get_default_instance(), 6000), LOG_ERR("Failed to Start CoAP"));
        LOG_INF("END: CoAP Start");

        return 0;
}