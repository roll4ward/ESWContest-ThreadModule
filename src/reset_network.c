#include "action_button.h"
#include "button_service.h"
#include "bluetooth_ad.h"
#include "coap.h"

#include <openthread/dataset.h>
#include <openthread/thread.h>

#include <zephyr/net/openthread.h>
#include <zephyr/logging/log.h>

#define BUTTON_NODE DT_ALIAS(sw0)

LOG_MODULE_REGISTER(reset_button, LOG_LEVEL_INF);

DEFINE_ACTION_BUTTON(reset_button, BUTTON_NODE, 5000);

BUTTON_CALLBACK(reset_thread) {
    otOperationalDataset dataset = {0};

    openthread_api_mutex_lock(openthread_get_default_context());
    otThreadSetEnabled(openthread_get_default_instance(), false);
    LOG_INF("Thread Disabled");
    otDatasetSetActive(openthread_get_default_instance(), &dataset);
    LOG_INF("Dataset reset");
    start_bt_advertise();
    LOG_INF("Start Advertisement");
    openthread_api_mutex_unlock(openthread_get_default_context());
}

struct action_button_callback reset_button_cb = {
    .on_long_press = reset_thread
};

K_WORK_DELAYABLE_DEFINE(reset_network_work, reset_thread);

static void reset_network(UserData *aUserData, double value) {
    k_work_schedule(&reset_network_work, K_TIMEOUT_ABS_MS(500));
}

DEFINE_COAP_USER_DATA_BUFFER(uint8_t, 5, reset_msg, reset_network);
DEFINE_COAP_RESOURCE(reset, &reset_msg);

void init_reset_network() {
    register_button(&reset_button, &reset_button_cb);
    memcpy(COAP_USER_DATA(reset_msg), "Done.", 5);
    addCoAPResource(&COAP_RESOURCE(reset));
}