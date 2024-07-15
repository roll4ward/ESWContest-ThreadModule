#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

#include <zephyr/net/openthread.h>
#include <openthread/thread.h>

#include "ble_commissioning.h"
#include "ble_uuid.h"

LOG_MODULE_REGISTER(ble_thread_scan, LOG_LEVEL_INF);

// User Data section
USER_DATA_INFO(otNetworkName, scan_networkname, OT_NETWORK_NAME_MAX_SIZE + 1, {0});
USER_DATA_INFO(otExtendedPanId, scan_extpanid, OT_EXT_PAN_ID_SIZE, {0});
USER_DATA_INFO(status, scan_status, 1U, WAITING);
USER_DATA_INFO(uint8_t, length, 1U, 0);

// Workqueue
COMMAND_WORK_DECLARE(scan);
COMMAND_WORK_DECLARE(reset_queue);
COMMAND_WORK_DECLARE(get_result);

// Scan Result Queue
K_QUEUE_DEFINE(scan_result_queue);
static void add_scan_result_to_queue(otActiveScanResult *aResult, void *aContext);

// Indicate Enable Variable
INDICATE_DEFINE(scan_networkname, BT_UUID_SCAN_NETWORK_NAME);
INDICATE_DEFINE(scan_extpanid, BT_UUID_SCAN_EXT_PANID);
INDICATE_DEFINE(scan_status, BT_UUID_SCAN_STATUS);
INDICATE_DEFINE(length, BT_UUID_SCAN_LENGTH);

static ssize_t write_command(struct bt_conn *conn,
                                  const struct bt_gatt_attr *attr,
                                  const void *buf, uint16_t len,
                                  uint16_t offset, uint8_t flags);

BT_GATT_SERVICE_DEFINE(
    thread_scan_service,
    BT_GATT_PRIMARY_SERVICE(BT_UUID_SCAN_SERVICE),
    BT_GATT_CHARACTERISTIC(BT_UUID_SCAN_COMMAND,
                           BT_GATT_CHRC_WRITE,
                           BT_GATT_PERM_WRITE,
                           NULL, write_command, NULL),
    BT_GATT_CHARACTERISTIC(BT_UUID_SCAN_NETWORK_NAME,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_INDICATE,
                           BT_GATT_PERM_READ,
                           read_gatt, NULL, &USER_DATA(scan_networkname)),
    BT_GATT_CCC(INDICATE_CCC_CALLBACK(scan_networkname), BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
    BT_GATT_CHARACTERISTIC(BT_UUID_SCAN_EXT_PANID,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_INDICATE,
                           BT_GATT_PERM_READ,
                           read_gatt, NULL, &USER_DATA(scan_extpanid)),
    BT_GATT_CCC(INDICATE_CCC_CALLBACK(scan_extpanid), BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),                       
    BT_GATT_CHARACTERISTIC(BT_UUID_SCAN_STATUS,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_INDICATE,
                           BT_GATT_PERM_READ,
                           read_gatt, NULL, &USER_DATA(scan_status)),
    BT_GATT_CCC(INDICATE_CCC_CALLBACK(scan_status), BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
    BT_GATT_CHARACTERISTIC(BT_UUID_SCAN_LENGTH,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_INDICATE,
                           BT_GATT_PERM_READ,
                           read_gatt, NULL, &USER_DATA(length)),
    BT_GATT_CCC(INDICATE_CCC_CALLBACK(length), BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
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
        case SCAN:
            LOG_DBG("COMMAND SET : SCAN");
            k_work_submit(&COMMAND_WORK_NAME(scan));
            break;

        case RESET_QUEUE:
            LOG_DBG("COMMAND SET : RESET_QUEUE");
            k_work_submit(&COMMAND_WORK_NAME(reset_queue));
            break;

        case GET:
            LOG_DBG("COMMAND SET : GET");
            k_work_submit(&COMMAND_WORK_NAME(get_result));
            break;

        default:
            return BT_GATT_ERR(BT_ATT_ERR_NOT_SUPPORTED);
            break;
    }

    return len;
}

COMMAND_WORK_HANDLER(scan) {
    otError err;
    
    LOG_DBG("Start Scan");
    INDICATE_VALUE(scan_status, PROGRESSING);

    otIp6SetEnabled(openthread_get_default_instance(), true);
    err = otThreadDiscover(openthread_get_default_instance(), 0xffffffff, OT_PANID_BROADCAST, false, false, add_scan_result_to_queue, NULL);

    if (err != OT_ERROR_NONE) {
        INDICATE_VALUE(scan_status, FAILED);
        LOG_ERR("SCAN FAILED : %d", err);
    }
}

static void add_scan_result_to_queue(otActiveScanResult *aResult, void *aContext) {
    LOG_DBG("Scan callback");

    LOG_HEXDUMP_DBG(&aResult->mNetworkName, sizeof(aResult->mNetworkName), "a Result Name");
    LOG_HEXDUMP_DBG(&aResult->mPanId, sizeof(aResult->mPanId), "a Panid");

    if (!aResult->mDiscover) {
        LOG_DBG("Scan end");
        INDICATE_VALUE(scan_status, DONE);
        return;
    }

    otActiveScanResult *result = k_malloc(sizeof(otActiveScanResult));
    if(!result) {
        LOG_ERR("CAN NOT Allocate memory for scan result");
        INDICATE_VALUE(scan_status, FAILED);
        return;
    }

    memcpy(result, aResult, sizeof(otActiveScanResult));

    k_queue_append(&scan_result_queue, result);

    ++(USER_DATA(length));
    INDICATE(length);

    LOG_DBG("Add Result : %s, length = %d", result->mNetworkName.m8, USER_DATA(length));
}

COMMAND_WORK_HANDLER(reset_queue) {
    LOG_DBG("Reset Queue");
    INDICATE_VALUE(scan_status, PROGRESSING);
    
    while (!k_queue_is_empty(&scan_result_queue)) {
        k_free(k_queue_get(&scan_result_queue, K_NO_WAIT));
        LOG_DBG("DELETE ITEM of QUEUE");
    }

    USER_DATA(length) = 0;
    INDICATE(length);

    INDICATE_VALUE(scan_status, DONE);

    LOG_DBG("RESET QUEUE DONE, length = %d", USER_DATA(length));
}

COMMAND_WORK_HANDLER(get_result) {
    LOG_DBG("get result");
    INDICATE_VALUE(scan_status, PROGRESSING);

    otActiveScanResult *result = k_queue_get(&scan_result_queue, K_NO_WAIT);
    if (!result) {
        INDICATE_VALUE(scan_status, FAILED);
        LOG_DBG("No Data in Queue, empty ? = %d", k_queue_is_empty(&scan_result_queue));
        return;
    }

    --USER_DATA(length);
    INDICATE(length);

    LOG_DBG("Get Result : %s, length = %d", result->mNetworkName.m8, USER_DATA(length));

    memcpy(&USER_DATA(scan_networkname), &result->mNetworkName, sizeof(otNetworkName));
    memcpy(&USER_DATA(scan_extpanid), &result->mExtendedPanId, sizeof(otExtendedPanId));
    k_free(result);

    INDICATE(scan_networkname);
    INDICATE(scan_extpanid);
    INDICATE_VALUE(scan_status, DONE);
}