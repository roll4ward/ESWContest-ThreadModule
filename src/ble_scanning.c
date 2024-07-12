#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

#include <zephyr/net/openthread.h>
#include <openthread/thread.h>

#include "ble_commissioning.h"

#define BT_UUID_SCAN_SERVICE_VAL                                 BT_UUID_128_ENCODE(0x9fff0008,0x89f6, 0x4f1b, 0x9e5d, 0x0d4648d944c9)
#define BT_UUID_SCAN_COMMAND_VAL                                 BT_UUID_128_ENCODE(0x9fff0009,0x89f6, 0x4f1b, 0x9e5d, 0x0d4648d944c9)
#define BT_UUID_SCAN_NETWORK_NAME_VAL                            BT_UUID_128_ENCODE(0x9fff000a,0x89f6, 0x4f1b, 0x9e5d, 0x0d4648d944c9)
#define BT_UUID_SCAN_EXT_PANID_VAL                               BT_UUID_128_ENCODE(0x9fff000b,0x89f6, 0x4f1b, 0x9e5d, 0x0d4648d944c9)
#define BT_UUID_SCAN_STATUS_VAL                                  BT_UUID_128_ENCODE(0x9fff000c,0x89f6, 0x4f1b, 0x9e5d, 0x0d4648d944c9)
#define BT_UUID_SCAN_LENGTH_VAL                                  BT_UUID_128_ENCODE(0x9fff000d,0x89f6, 0x4f1b, 0x9e5d, 0x0d4648d944c9)

#define BT_UUID_SCAN_SERVICE                                     BT_UUID_DECLARE_128(BT_UUID_SCAN_SERVICE_VAL)
#define BT_UUID_SCAN_COMMAND                                     BT_UUID_DECLARE_128(BT_UUID_SCAN_COMMAND_VAL)
#define BT_UUID_SCAN_NETWORK_NAME                                BT_UUID_DECLARE_128(BT_UUID_SCAN_NETWORK_NAME_VAL)
#define BT_UUID_SCAN_EXT_PANID                                   BT_UUID_DECLARE_128(BT_UUID_SCAN_EXT_PANID_VAL)
#define BT_UUID_SCAN_STATUS                                      BT_UUID_DECLARE_128(BT_UUID_SCAN_STATUS_VAL)
#define BT_UUID_SCAN_LENGTH                                      BT_UUID_DECLARE_128(BT_UUID_SCAN_LENGTH_VAL)

LOG_MODULE_REGISTER(ble_thread_scan, LOG_LEVEL_INF);

static void scan(struct k_work *work);
static void reset_queue(struct k_work *work);
static void get_result(struct k_work *work);

static void add_scan_result_to_queue(otActiveScanResult *aResult, void *aContext);

K_WORK_DEFINE(scan_work, scan);
K_WORK_DEFINE(reset_queue_work, reset_queue);
K_WORK_DEFINE(get_result_work, get_result);

K_QUEUE_DEFINE(scan_result_queue);

// User Datasection
USER_DATA_INFO(otNetworkName, scan_networkname, OT_NETWORK_NAME_MAX_SIZE + 1, {0});
USER_DATA_INFO(otExtendedPanId, scan_extpanid, OT_EXT_PAN_ID_SIZE, {0});
USER_DATA_INFO(status, scan_status, 1U, WAITING);
USER_DATA_INFO(uint8_t, length, 1U, 0);

static bool network_name_indicate_enabled = false;
static bool ext_panid_indicate_enabled = false;
static bool status_indicate_enabled = false;
static bool length_indicate_enabled = false;

static struct bt_gatt_indicate_params network_name_ind_params = {
    .uuid = BT_UUID_SCAN_NETWORK_NAME,
    .func = NULL,
    .destroy = NULL,
    .data = &USER_DATA_ORIGIN(scan_networkname),
    .len = USER_DATA_LENGTH(scan_networkname)
};

static struct bt_gatt_indicate_params ext_panid_ind_params = {
    .uuid = BT_UUID_SCAN_EXT_PANID,
    .func = NULL,
    .destroy = NULL,
    .data = &USER_DATA_ORIGIN(scan_extpanid),
    .len = USER_DATA_LENGTH(scan_extpanid)
};

static struct bt_gatt_indicate_params status_ind_params = {
    .uuid = BT_UUID_SCAN_STATUS,
    .func = NULL,
    .destroy = NULL,
    .data = &USER_DATA_ORIGIN(scan_status),
    .len = USER_DATA_LENGTH(scan_status)
};

static struct bt_gatt_indicate_params length_ind_params = {
    .uuid = BT_UUID_SCAN_LENGTH,
    .func = NULL,
    .destroy = NULL,
    .data = &USER_DATA_ORIGIN(length),
    .len = USER_DATA_LENGTH(length)
};

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
            k_work_submit(&scan_work);
            break;

        case RESET_QUEUE:
            LOG_DBG("COMMAND SET : RESET_QUEUE");
            k_work_submit(&reset_queue_work);
            break;

        case GET:
            LOG_DBG("COMMAND SET : GET");
            k_work_submit(&get_result_work);
            break;

        default:
            return BT_GATT_ERR(BT_ATT_ERR_NOT_SUPPORTED);
            break;
    }

    return len;
}

static void network_name_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value) {
    network_name_indicate_enabled = (value == BT_GATT_CCC_INDICATE);
}

static void ext_panid_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value) {
    ext_panid_indicate_enabled = (value == BT_GATT_CCC_INDICATE);
}

static void status_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value) {
    status_indicate_enabled = (value == BT_GATT_CCC_INDICATE);
}

static void length_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value) {
    length_indicate_enabled = (value == BT_GATT_CCC_INDICATE);
}

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
                           read_gatt, NULL, &USER_DATA_ORIGIN(scan_networkname)),
    BT_GATT_CCC(network_name_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
    BT_GATT_CHARACTERISTIC(BT_UUID_SCAN_EXT_PANID,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_INDICATE,
                           BT_GATT_PERM_READ,
                           read_gatt, NULL, &USER_DATA_ORIGIN(scan_extpanid)),
    BT_GATT_CCC(ext_panid_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),                       
    BT_GATT_CHARACTERISTIC(BT_UUID_SCAN_STATUS,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_INDICATE,
                           BT_GATT_PERM_READ,
                           read_gatt, NULL, &USER_DATA_ORIGIN(scan_status)),
    BT_GATT_CCC(status_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
    BT_GATT_CHARACTERISTIC(BT_UUID_SCAN_LENGTH,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_INDICATE,
                           BT_GATT_PERM_READ,
                           read_gatt, NULL, &USER_DATA_ORIGIN(length)),
    BT_GATT_CCC(length_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

static int network_name_indicate() {
    if (!network_name_indicate_enabled) {
		return -EACCES;
	}

	return bt_gatt_indicate(NULL, &network_name_ind_params);
}

static int ext_panid_indicate() {
    if (!ext_panid_indicate_enabled) {
		return -EACCES;
	}

	return bt_gatt_indicate(NULL, &ext_panid_ind_params);
}

static int status_indicate(uint8_t state) {
    USER_DATA_ORIGIN(scan_status) = state;

    if (!status_indicate_enabled) {
		return -EACCES;
	}

	return bt_gatt_indicate(NULL, &status_ind_params);
}

static int length_indicate() {
    if (!length_indicate_enabled) {
		return -EACCES;
	}

	return bt_gatt_indicate(NULL, &length_ind_params);
}

static void scan(struct k_work *work) {
    otError err;
    
    LOG_DBG("Start Scan");
    status_indicate(PROGRESSING);

    otIp6SetEnabled(openthread_get_default_instance(), true);
    err = otThreadDiscover(openthread_get_default_instance(), 0xffffffff, OT_PANID_BROADCAST, false, false, add_scan_result_to_queue, NULL);

    if (err != OT_ERROR_NONE) {
        status_indicate(FAILED);
        LOG_ERR("SCAN FAILED : %d", err);
    }
}

static void add_scan_result_to_queue(otActiveScanResult *aResult, void *aContext) {
    LOG_DBG("Scan callback");

    LOG_HEXDUMP_DBG(&aResult->mNetworkName, sizeof(aResult->mNetworkName), "a Result Name");
    LOG_HEXDUMP_DBG(&aResult->mPanId, sizeof(aResult->mPanId), "a Panid");

    if (!aResult->mDiscover) {
        LOG_DBG("Scan end");
        status_indicate(DONE);
        return;
    }

    otActiveScanResult *result = k_malloc(sizeof(otActiveScanResult));
    if(!result) {
        LOG_ERR("CAN NOT Allocate memory for scan result");
        status_indicate(FAILED);
        return;
    }

    memcpy(result, aResult, sizeof(otActiveScanResult));

    k_queue_append(&scan_result_queue, result);

    ++(USER_DATA_ORIGIN(length));
    length_indicate();

    LOG_DBG("Add Result : %s, length = %d", result->mNetworkName.m8, USER_DATA_ORIGIN(length));
}

static void reset_queue(struct k_work *work) {
    LOG_DBG("Reset Queue");
    status_indicate(PROGRESSING);
    
    while (!k_queue_is_empty(&scan_result_queue)) {
        k_free(k_queue_get(&scan_result_queue, K_NO_WAIT));
        LOG_DBG("DELETE ITEM of QUEUE");
    }

    USER_DATA_ORIGIN(length) = 0;
    length_indicate();

    status_indicate(DONE);
    LOG_DBG("RESET QUEUE DONE, length = %d", USER_DATA_ORIGIN(length));
}

static void get_result(struct k_work *work) {
    LOG_DBG("get result");
    status_indicate(PROGRESSING);

    otActiveScanResult *result = k_queue_get(&scan_result_queue, K_NO_WAIT);
    if (!result) {
        status_indicate(FAILED);
        LOG_DBG("No Data in Queue, empty ? = %d", k_queue_is_empty(&scan_result_queue));
        return;
    }

    --USER_DATA_ORIGIN(length);
    length_indicate();

    LOG_DBG("Get Result : %s, length = %d", result->mNetworkName.m8, USER_DATA_ORIGIN(length));

    memcpy(&USER_DATA_ORIGIN(scan_networkname), &result->mNetworkName, sizeof(otNetworkName));
    memcpy(&USER_DATA_ORIGIN(scan_extpanid), &result->mExtendedPanId, sizeof(otExtendedPanId));
    k_free(result);

    network_name_indicate();
    ext_panid_indicate();
    status_indicate(DONE);
}