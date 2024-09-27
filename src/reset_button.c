#include "action_button.h"
#include "bluetooth_ad.h"

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