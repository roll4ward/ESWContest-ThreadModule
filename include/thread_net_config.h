#ifndef ROLL4_THREAD_NET_CONFIG_H
#define ROLL4_THREAD_NET_CONFIG_H

#include <openthread/dataset.h>
#include <openthread/thread.h>
#include <openthread/link.h>
#include <openthread/dataset.h>

typedef struct network_info {
    uint8_t channel;
    otPanId pan_id;
    otExtendedPanId *extended_pan_id;
    otNetworkKey *network_key;
    const char *network_name;
} network_info;

otError commit_dataset(network_info *info);

#endif