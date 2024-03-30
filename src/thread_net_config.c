#include "thread_net_config.h"
#include <zephyr/net/openthread.h>
#include <openthread/link.h>
#include <openthread/thread.h>

static otError set_info_to_otInstance(network_info *info, otInstance *instance);

otError commit_dataset(network_info *info) {
    otError err = OT_ERROR_NONE;

    struct openthread_context* otContext = openthread_get_default_context();
    openthread_api_mutex_lock(otContext);
    otThreadSetEnabled(otContext->instance, false);

    err = set_info_to_otInstance(otContext->instance, info);

    openthread_api_mutex_unlock(otContext);
    openthread_start(otContext);

    return err;
}

static otError set_info_to_otInstance(network_info *info, otInstance *instance) {
    otError err = OT_ERROR_NONE;
    
    otLinkSetPanId(instance, info->pan_id);
    err = otLinkSetChannel(instance, info->channel);
    if (err == OT_ERROR_INVALID_ARGS) return err;

    otThreadSetExtendedPanId(instance, info->extended_pan_id);
    otThreadSetNetworkKey(instance, info->network_key);
    otThreadSetNetworkName(instance, info->network_name);

    return err;
}