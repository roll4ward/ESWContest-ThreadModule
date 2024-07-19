#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

#include <zephyr/net/openthread.h>
#include <openthread/dataset.h>
#include <openthread/dataset_ftd.h>
#include <openthread/thread.h>
#include <openthread/instance.h>

#include "ble_commissioning.h"
#include "ble_uuid.h"

LOG_MODULE_REGISTER(ble_create_network, LOG_LEVEL_DBG);

// User Data Section

// User Data Info
USER_DATA_INFO(otNetworkName, networkname, OT_NETWORK_NAME_MAX_SIZE + 1, {0});
USER_DATA_INFO(otNetworkKey, networkkey, OT_NETWORK_KEY_SIZE, {0});
USER_DATA_INFO(otExtendedPanId, extpanid, OT_EXT_PAN_ID_SIZE, {0});
USER_DATA_INFO(status, commission_status, 1U, WAITING);
USER_DATA_INFO(otDeviceRole, role, 1U, OT_DEVICE_ROLE_DISABLED);

// Indicate Enable
INDICATE_DEFINE(commission_status, BT_UUID_COMMISSION_STATUS);
INDICATE_DEFINE(role, BT_UUID_COMMISSION_ROLE);

// Workqueue Variables
COMMAND_WORK_DECLARE(create_new_network);
COMMAND_WORK_DECLARE(join_network);
COMMAND_WORK_DECLARE(reset_dataset);

static ssize_t write_command(struct bt_conn *conn,
                                  const struct bt_gatt_attr *attr,
                                  const void *buf, uint16_t len,
                                  uint16_t offset, uint8_t flags);

BT_GATT_SERVICE_DEFINE(
    thread_commission_service,
    BT_GATT_PRIMARY_SERVICE(BT_UUID_COMMISSION_SERVICE),
    BT_GATT_CHARACTERISTIC(BT_UUID_COMMISSION_NETWORK_NAME,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
                           BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
                           read_gatt, write_gatt, &networkname),
    BT_GATT_CHARACTERISTIC(BT_UUID_COMMISSION_NETWORK_KEY,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
                           BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
                           read_gatt, write_gatt, &networkkey),
    BT_GATT_CHARACTERISTIC(BT_UUID_COMMISSION_EXT_PANID,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
                           BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
                           read_gatt, write_gatt, &extpanid),
    BT_GATT_CHARACTERISTIC(BT_UUID_COMMISSION_COMMAND,
                           BT_GATT_CHRC_WRITE,
                           BT_GATT_PERM_WRITE,
                           NULL, write_command, NULL),
    BT_GATT_CHARACTERISTIC(BT_UUID_COMMISSION_STATUS,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_INDICATE,
                           BT_GATT_PERM_READ,
                           read_gatt, NULL, &commission_status),
    BT_GATT_CCC(INDICATE_CCC_CALLBACK(commission_status), BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),                      
    BT_GATT_CHARACTERISTIC(BT_UUID_COMMISSION_ROLE,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_INDICATE,
                           BT_GATT_PERM_READ,
                           read_gatt, NULL, &role),
    BT_GATT_CCC(INDICATE_CCC_CALLBACK(role), BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

static ssize_t write_command(struct bt_conn *conn,
                                  const struct bt_gatt_attr *attr,
                                  const void *buf, uint16_t len,
                                  uint16_t offset, uint8_t flags) {
    if (len != 1U) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
    }

    if (offset != 0) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }

    switch (*((uint8_t *)buf)) {
        case NEW_NETWORK:
            LOG_DBG("COMMAND SET : CREATE NEW");
            k_work_submit(&COMMAND_WORK_NAME(create_new_network));
            break;

        case JOIN_NETWORK:
            LOG_DBG("COMMAND SET : JOIN");
            k_work_submit(&COMMAND_WORK_NAME(join_network));
            break;

        case RESET_DATASET:
            LOG_DBG("COMMAND SET : RESET");
            k_work_submit(&COMMAND_WORK_NAME(reset_dataset));
            break;

        default:
            return BT_GATT_ERR(BT_ATT_ERR_NOT_SUPPORTED);
            break;
    }

    return len;
}

COMMAND_WORK_HANDLER(create_new_network) {
    otOperationalDataset new_dataset;
    otError err;
    
    openthread_api_mutex_lock(openthread_get_default_context());
    otIp6SetEnabled(openthread_get_default_instance(), true);
    otThreadSetEnabled(openthread_get_default_instance(), false);

    LOG_DBG("Let's Create New NETWORK");
    INDICATE_VALUE(commission_status, PROGRESSING);

    err = otDatasetCreateNewNetwork(openthread_get_default_instance(), &new_dataset);
    
    if (err != OT_ERROR_NONE) {
        LOG_ERR("FAILED: Create NEW Network");
        INDICATE_VALUE(commission_status, FAILED);
        return;
    }

    memcpy(&new_dataset.mNetworkName, &USER_DATA(networkname), sizeof(otNetworkName));
    memcpy(&USER_DATA(networkkey), &new_dataset.mNetworkKey, sizeof(otNetworkKey));
    memcpy(&USER_DATA(extpanid), &new_dataset.mExtendedPanId, sizeof(otExtendedPanId));

    err = otDatasetSetActive(openthread_get_default_instance(), &new_dataset);

    if (err != OT_ERROR_NONE) {
        LOG_ERR("FAILED: Create NEW Network");
        INDICATE_VALUE(commission_status, FAILED);
        return;
    }

    LOG_DBG("Done: Create New Network");
    otThreadSetEnabled(openthread_get_default_instance(), true);
    openthread_api_mutex_unlock(openthread_get_default_context());

    INDICATE_VALUE(commission_status, DONE);
}

COMMAND_WORK_HANDLER(join_network) {
    otError err;
    otOperationalDataset join_dataset = {0};
    
    openthread_api_mutex_lock(openthread_get_default_context());
    otIp6SetEnabled(openthread_get_default_instance(), true);
    otThreadSetEnabled(openthread_get_default_instance(), false);

    LOG_DBG("Let's Join New NETWORK");
    INDICATE_VALUE(commission_status, PROGRESSING);

    memcpy(&join_dataset.mNetworkName, &USER_DATA(networkname), sizeof(otNetworkName));
    join_dataset.mComponents.mIsNetworkNamePresent = true;
    memcpy(&join_dataset.mNetworkKey, &USER_DATA(networkkey), sizeof(otNetworkKey));
    join_dataset.mComponents.mIsNetworkKeyPresent = true;
    memcpy(&join_dataset.mExtendedPanId, &USER_DATA(extpanid), sizeof(otExtendedPanId));
    join_dataset.mComponents.mIsExtendedPanIdPresent = true;

    err = otDatasetSetActive(openthread_get_default_instance(), &join_dataset);
    if (err != OT_ERROR_NONE) {
        LOG_ERR("cannot update DATASET");
        INDICATE_VALUE(commission_status, FAILED);
        return;
    }

    LOG_DBG("Done: Join New Network");

    otThreadSetEnabled(openthread_get_default_instance(), true);
    openthread_api_mutex_unlock(openthread_get_default_context());

    INDICATE_VALUE(commission_status, DONE);
}

COMMAND_WORK_HANDLER(reset_dataset) {
    INDICATE_VALUE(commission_status, PROGRESSING);

    memset(&USER_DATA(networkname), 0, sizeof(otNetworkName));
    memset(&USER_DATA(networkkey), 0, sizeof(otNetworkKey));
    memset(&USER_DATA(extpanid), 0, sizeof(otExtendedPanId));

    otThreadSetEnabled(openthread_get_default_instance, false);
    
    INDICATE_VALUE(commission_status, DONE);
}

static void state_changed_cb(otChangedFlags flag, void *context) {
    if (flag & OT_CHANGED_THREAD_ROLE) {
        INDICATE_VALUE(role, otThreadGetDeviceRole(openthread_get_default_instance()));
    }
}

void init_ble_commission() {
    if (otSetStateChangedCallback(openthread_get_default_instance(), state_changed_cb, NULL) != OT_ERROR_NONE) {
        LOG_ERR("CALL BACK Register failed");
        return;
    }
}