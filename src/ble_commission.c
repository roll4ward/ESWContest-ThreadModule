#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

#include <zephyr/net/openthread.h>
#include <openthread/dataset.h>
#include <openthread/dataset_ftd.h>
#include <openthread/thread.h>
#include <openthread/instance.h>

#include "ble_commissioning.h"
#include "ble_uuid.h"
#include "bluetooth_ad.h"

LOG_MODULE_REGISTER(ble_commission, LOG_LEVEL_DBG);

// User Data Info
USER_DATA_INFO(otNetworkKey, networkkey, OT_NETWORK_KEY_SIZE, {0});
USER_DATA_INFO(status, commission_status, 1U, WAITING);
USER_DATA_INFO(otDeviceRole, role, 1U, OT_DEVICE_ROLE_DISABLED);
USER_DATA_INFO(otIp6Address, ipv6_address, sizeof(otIp6Address), {0});

// Indicate Enable
INDICATE_DEFINE(commission_status, BT_UUID_COMMISSION_STATUS);
INDICATE_DEFINE(role, BT_UUID_COMMISSION_ROLE);
INDICATE_DEFINE(ipv6_address, BT_UUID_COMMISSION_ML_ADDR);

// Workqueue Variables
COMMAND_WORK_DECLARE(join_network);
COMMAND_WORK_DECLARE(reset_dataset);
COMMAND_WORK_DECLARE(disable_ble);

static void disconnect_conn(struct bt_conn * connect, void * data);
static ssize_t write_command(struct bt_conn *conn,
                                  const struct bt_gatt_attr *attr,
                                  const void *buf, uint16_t len,
                                  uint16_t offset, uint8_t flags);

BT_GATT_SERVICE_DEFINE(
    thread_commission_service,
    BT_GATT_PRIMARY_SERVICE(BT_UUID_COMMISSION_SERVICE),
    BT_GATT_CHARACTERISTIC(BT_UUID_COMMISSION_NETWORK_KEY,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
                           BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
                           read_gatt, write_gatt, &networkkey),
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
    BT_GATT_CHARACTERISTIC(BT_UUID_COMMISSION_ML_ADDR,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_INDICATE,
                           BT_GATT_PERM_READ,
                           read_gatt, NULL, &ipv6_address),
    BT_GATT_CCC(INDICATE_CCC_CALLBACK(ipv6_address), BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
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
        case JOIN_NETWORK:
            LOG_DBG("COMMAND SET : JOIN");
            k_work_submit(&COMMAND_WORK_NAME(join_network));
            break;

        case RESET_DATASET:
            LOG_DBG("COMMAND SET : RESET");
            k_work_submit(&COMMAND_WORK_NAME(reset_dataset));
            break;
        
        case DISABLE_BLE:
            LOG_DBG("COMMAND SET : DISABLE BLE");
            k_work_submit(&COMMAND_WORK_NAME(disable_ble));
            break;

        default:
            return BT_GATT_ERR(BT_ATT_ERR_NOT_SUPPORTED);
            break;
    }

    return len;
}

COMMAND_WORK_HANDLER(join_network) {
    otError err;
    otOperationalDataset join_dataset = {0};
    
    openthread_api_mutex_lock(openthread_get_default_context());
    otIp6SetEnabled(openthread_get_default_instance(), true);
    otThreadSetEnabled(openthread_get_default_instance(), false);

    LOG_DBG("Let's Join New NETWORK");
    INDICATE_VALUE(commission_status, PROGRESSING);

    memcpy(&join_dataset.mNetworkKey, &USER_DATA(networkkey), sizeof(otNetworkKey));
    join_dataset.mComponents.mIsNetworkKeyPresent = true;

    err = otDatasetSetActive(openthread_get_default_instance(), &join_dataset);
    if (err != OT_ERROR_NONE) {
        LOG_ERR("cannot update DATASET");
        INDICATE_VALUE(commission_status, FAILED);
        return;
    }

    LOG_DBG("Done: Join New Network");

    otThreadSetEnabled(openthread_get_default_instance(), true);
    LOG_DBG("DONE: Enable Thread");
    openthread_api_mutex_unlock(openthread_get_default_context());
    LOG_DBG("DONE : Mutex unlock");
    INDICATE_VALUE(commission_status, DONE);
    LOG_DBG("DONE : INDICATE");
}

COMMAND_WORK_HANDLER(reset_dataset) {
    INDICATE_VALUE(commission_status, PROGRESSING);

    memset(&USER_DATA(networkkey), 0, sizeof(otNetworkKey));
    
    otThreadSetEnabled(openthread_get_default_instance(), false);
    
    INDICATE_VALUE(commission_status, DONE);
}

COMMAND_WORK_HANDLER(disable_ble) {
    stop_bt_advertise();
    bt_conn_foreach(BT_CONN_TYPE_ALL, disconnect_conn, NULL);
}

static void disconnect_conn(struct bt_conn * connect, void * data) {
    bt_conn_disconnect(connect, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
}

static void state_changed_cb(otChangedFlags flag, void *context) {
    if (flag & OT_CHANGED_THREAD_ROLE) {
        LOG_DBG("ROLE changed");
        INDICATE_VALUE(role, otThreadGetDeviceRole(openthread_get_default_instance()));
        LOG_DBG("ROLE indicated");
    }

    if (flag & OT_CHANGED_THREAD_ML_ADDR) {
        LOG_DBG("ML ADDR changed");
        memcpy(&USER_DATA(ipv6_address), otThreadGetMeshLocalEid(openthread_get_default_instance()), sizeof(otIp6Address));
        INDICATE(ipv6_address);
    }
}

void init_ble_commission() {
    if (otSetStateChangedCallback(openthread_get_default_instance(), state_changed_cb, NULL) != OT_ERROR_NONE) {
        LOG_ERR("CALL BACK Register failed");
        return;
    }

    USER_DATA(role) = otThreadGetDeviceRole(openthread_get_default_instance());
    memcpy(&USER_DATA(ipv6_address), otThreadGetMeshLocalEid(openthread_get_default_instance()), sizeof(otIp6Address));
    
    LOG_INF("DONE : Init ble");
}