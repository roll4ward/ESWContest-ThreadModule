#include "thread_net_config.h"
#include <zephyr/shell/shell.h>
#include <zephyr/net/openthread.h>
#include <openthread/dataset.h>
#include <openthread/thread.h>
#include <openthread/link.h>

otExtendedPanId extpanid;
otNetworkKey netkey;
otNetworkName netname;

static network_info info = {
    .channel = 11,
    .pan_id = 0x1234,
    .extended_pan_id = &extpanid,
    .network_name = NULL,
    .network_key = &netkey
};



static int channel_cmd(const struct shell *sh, size_t argc, char **argv) { 
    if (argc > 1) {
        info.channel = atoi(argv[1]);
        shell_print(sh, "Set Channel : %d", info.channel);
        return 0;
    }

    else if(argc == 1) {
        shell_print(sh, "Channel : %d", info.channel);
        return 0;
    }

    return 1;
}

static int panid_cmd(const struct shell *sh, size_t argc, char **argv) { 
    if (argc > 1) {
        info.pan_id = atoi(argv[1]);
        shell_print(sh, "Set PAN ID : %d", info.pan_id);
        return 0;
    }

    else if(argc == 1) {
        shell_print(sh, "PAN ID : %d", info.pan_id);
        return 0;
    }

    return 1;
}

static int commit_config(const struct shell *sh, size_t argc, char **argv) { 
    otError err = commit_dataset(&info);

    shell_print(sh, "Done. : %d", err);
    return 0;
}

static int current_dataset(const struct shell *sh, size_t argc, char **argv) { 
    otOperationalDataset dataset;
    otDatasetGetActive(openthread_get_default_instance(), &dataset);

    shell_print(sh, "Channel : %d", dataset.mChannel);
    shell_print(sh, "PAN ID : %d", dataset.mPanId);

    shell_print(sh, "Real Channel : %d", otLinkGetChannel(openthread_get_default_instance()));
    shell_print(sh, "Real PAN ID : %d", otLinkGetPanId(openthread_get_default_instance()));
    
    return 0;
}

static int init_info(const struct shell *sh, size_t argc, char **argv) { 
    otOperationalDataset dataset;
    otDatasetGetActive(openthread_get_default_instance(), &dataset);

    info.channel = dataset.mChannel;
    info.pan_id = dataset.mPanId;
    *info.extended_pan_id = dataset.mExtendedPanId;
    info.network_name = dataset.mNetworkName.m8;
    *info.network_key = dataset.mNetworkKey;

    shell_print(sh, "Done.");
    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(netconfig_sub,
    SHELL_CMD(channel, NULL, "set channel", channel_cmd),
    SHELL_CMD(panid, NULL, "set PAN ID", panid_cmd),
    SHELL_CMD(commit, NULL, "commit", commit_config),
    SHELL_CMD(active, NULL, "get active", current_dataset),
    SHELL_CMD(init, NULL, "init network_info", init_info),
);

SHELL_CMD_REGISTER(netconfig, &netconfig_sub, "Network Config", NULL);