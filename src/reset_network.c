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

static void reset_network(UserData *aUserData) {
    *(uint8_t *)(aUserData->mUserData) += 1;
    k_work_schedule(&reset_network_work, K_TIMEOUT_ABS_MS(1500));
}

DEFINE_COAP_USER_DATA(uint8_t, reset_num, reset_network);
DEFINE_COAP_RESOURCE(reset, &reset_num);

void init_reset_network() {
    register_button(&reset_button, &reset_button_cb);
    addCoAPResource(&COAP_RESOURCE(reset));
}