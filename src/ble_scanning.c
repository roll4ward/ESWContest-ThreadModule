#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

#include <zephyr/net/openthread.h>
#include <openthread/thread.h>

#include "ble_commissioning.h"

#define BT_UUID_SCAN_SERVICE_VAL                                 BT_UUID_128_ENCODE(0x9fff0008,0x89f6, 0x4f1b, 0x9e5d, 0xd4648d944c9)
#define BT_UUID_SCAN_COMMAND_VAL                                 BT_UUID_128_ENCODE(0x9fff0009,0x89f6, 0x4f1b, 0x9e5d, 0xd4648d944c9)
#define BT_UUID_SCAN_NETWORK_NAME_VAL                            BT_UUID_128_ENCODE(0x9fff000a,0x89f6, 0x4f1b, 0x9e5d, 0xd4648d944c9)
#define BT_UUID_SCAN_EXT_PANID_VAL                               BT_UUID_128_ENCODE(0x9fff000b,0x89f6, 0x4f1b, 0x9e5d, 0xd4648d944c9)
#define BT_UUID_SCAN_STATUS_VAL                                  BT_UUID_128_ENCODE(0x9fff000c,0x89f6, 0x4f1b, 0x9e5d, 0xd4648d944c9)
#define BT_UUID_SCAN_LENGTH_VAL                                  BT_UUID_128_ENCODE(0x9fff000d,0x89f6, 0x4f1b, 0x9e5d, 0xd4648d944c9)

#define BT_UUID_SCAN_SERVICE                                     BT_UUID_DECLARE_128(BT_UUID_SCAN_SERVICE_VAL)
#define BT_UUID_SCAN_COMMAND                                     BT_UUID_DECLARE_128(BT_UUID_SCAN_COMMAND_VAL)
#define BT_UUID_SCAN_NETWORK_NAME                                BT_UUID_DECLARE_128(BT_UUID_SCAN_NETWORK_NAME_VAL)
#define BT_UUID_SCAN_EXT_PANID                                   BT_UUID_DECLARE_128(BT_UUID_SCAN_EXT_PANID_VAL)
#define BT_UUID_SCAN_STATUS                                      BT_UUID_DECLARE_128(BT_UUID_SCAN_STATUS_VAL)
#define BT_UUID_SCAN_LENGTH                                      BT_UUID_DECLARE_128(BT_UUID_SCAN_LENGTH_VAL)

#define CMD_SCAN                                                 0x01
#define CMD_RESET_QUEUE                                          0X02
#define CMD_GET                                                  0X03

LOG_MODULE_REGISTER(ble_thread_scan, LOG_LEVEL_DBG);

static void scan(struct k_work *work);
static void reset_queue(struct k_work *work);
static void get_result(struct k_work *work);

K_WORK_DEFINE(scan_work, scan);
K_WORK_DEFINE(reset_queue_work, reset_queue);
K_WORK_DEFINE(get_result_work, get_result);

K_QUEUE_DEFINE(scan_result_queue);

static otActiveScanResult current_scan_result;

static uint8_t status = 0x00;
static uint32_t length = 0;

static bool network_name_indicate_enabled = false;
static bool ext_panid_indicate_enabled = false;
static bool status_indicate_enabled = false;
static bool length_indicate_enabled = false;

static struct bt_gatt_indicate_params network_name_ind_params = {
    .uuid = BT_UUID_SCAN_NETWORK_NAME,
    .func = NULL,
    .destroy = NULL,
    .data = current_scan_result.mNetworkName.m8,
    .len = sizeof(current_scan_result.mNetworkName.m8)
};

static struct bt_gatt_indicate_params ext_panid_ind_params = {
    .uuid = BT_UUID_SCAN_EXT_PANID,
    .func = NULL,
    .destroy = NULL,
    .data = current_scan_result.mExtendedPanId.m8,
    .len = sizeof(current_scan_result.mExtendedPanId.m8)
};

static struct bt_gatt_indicate_params status_ind_params = {
    .uuid = BT_UUID_SCAN_STATUS,
    .func = NULL,
    .destroy = NULL,
    .data = &status,
    .len = sizeof(status)
};

static struct bt_gatt_indicate_params length_ind_params = {
    .uuid = BT_UUID_SCAN_LENGTH,
    .func = NULL,
    .destroy = NULL,
    .data = &length,
    .len = sizeof(length)
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
        case CMD_SCAN:
            LOG_DBG("COMMAND SET : SCAN");
            k_work_submit_to_queue(get_commission_work_q(), &scan_work);
            break;

        case CMD_RESET_QUEUE:
            LOG_DBG("COMMAND SET : RESET_QUEUE");
            k_work_submit_to_queue(get_commission_work_q(), &reset_queue_work);
            break;

        case CMD_GET:
            LOG_DBG("COMMAND SET : GET");
            k_work_submit_to_queue(get_commission_work_q(), &get_result_work);
            break;

        default:
            return BT_GATT_ERR(BT_ATT_ERR_NOT_SUPPORTED);
            break;
    }

    return len;
}

extern ssize_t read_network_name(struct bt_conn *conn,
                                 const struct bt_gatt_attr *attr,
                                 void *buf, uint16_t len, 
                                 uint16_t offset);

extern ssize_t read_ext_panid(struct bt_conn *conn,
                                 const struct bt_gatt_attr *attr,
                                 void *buf, uint16_t len, 
                                 uint16_t offset);      

extern ssize_t read_status(struct bt_conn *conn,
                                 const struct bt_gatt_attr *attr,
                                 void *buf, uint16_t len, 
                                 uint16_t offset);

static ssize_t read_length(struct bt_conn *conn,
                                 const struct bt_gatt_attr *attr,
                                 void *buf, uint16_t len, 
                                 uint16_t offset) {
    uint32_t *value = attr->user_data;

    LOG_DBG("status read");
    
    return bt_gatt_attr_read(conn, attr, buf, len, offset, value, sizeof(*value));
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
                           read_network_name, NULL, current_scan_result.mNetworkName.m8),
    BT_GATT_CCC(network_name_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
    BT_GATT_CHARACTERISTIC(BT_UUID_SCAN_EXT_PANID,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_INDICATE,
                           BT_GATT_PERM_READ,
                           read_ext_panid, NULL, current_scan_result.mExtendedPanId.m8),
    BT_GATT_CCC(ext_panid_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),                       
    BT_GATT_CHARACTERISTIC(BT_UUID_SCAN_STATUS,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_INDICATE,
                           BT_GATT_PERM_READ,
                           read_status, NULL, &status),
    BT_GATT_CCC(status_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
    BT_GATT_CHARACTERISTIC(BT_UUID_SCAN_LENGTH,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_INDICATE,
                           BT_GATT_PERM_READ,
                           read_length, NULL, &length),
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

static int status_indicate() {
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
    LOG_DBG("Start Scan");
}

static void reset_queue(struct k_work *work) {
    LOG_DBG("Reset Queue");
}

static void get_result(struct k_work *work) {
    LOG_DBG("get result");
}