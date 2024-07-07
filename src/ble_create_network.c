#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

#include <zephyr/net/openthread.h>
#include <openthread/dataset.h>
#include <openthread/dataset_ftd.h>

#include "ble_commissioning.h"

#define BT_UUID_NEW_NETWORK_SERVICE_VAL                          BT_UUID_128_ENCODE(0x9fff0001,0x89f6, 0x4f1b, 0x9e5d, 0xd4648d944c9)
#define BT_UUID_NEW_NETWORK_NETWORK_NAME_VAL                     BT_UUID_128_ENCODE(0x9fff0002,0x89f6, 0x4f1b, 0x9e5d, 0xd4648d944c9)
#define BT_UUID_NEW_NETWORK_NETWORK_KEY_VAL                      BT_UUID_128_ENCODE(0x9fff0003,0x89f6, 0x4f1b, 0x9e5d, 0xd4648d944c9)
#define BT_UUID_NEW_NETWORK_EXT_PANID_VAL                        BT_UUID_128_ENCODE(0x9fff0004,0x89f6, 0x4f1b, 0x9e5d, 0xd4648d944c9)
#define BT_UUID_NEW_NETWORK_COMMAND_VAL                          BT_UUID_128_ENCODE(0x9fff0005,0x89f6, 0x4f1b, 0x9e5d, 0xd4648d944c9)
#define BT_UUID_NEW_NETWORK_STATUS_VAL                           BT_UUID_128_ENCODE(0x9fff0006,0x89f6, 0x4f1b, 0x9e5d, 0xd4648d944c9)

#define BT_UUID_NEW_NETWORK_SERVICE                              BT_UUID_DECLARE_128(BT_UUID_NEW_NETWORK_SERVICE_VAL)
#define BT_UUID_NEW_NETWORK_NETWORK_NAME                         BT_UUID_DECLARE_128(BT_UUID_NEW_NETWORK_NETWORK_NAME_VAL)
#define BT_UUID_NEW_NETWORK_NETWORK_KEY                          BT_UUID_DECLARE_128(BT_UUID_NEW_NETWORK_NETWORK_KEY_VAL)
#define BT_UUID_NEW_NETWORK_EXT_PANID                            BT_UUID_DECLARE_128(BT_UUID_NEW_NETWORK_EXT_PANID_VAL)
#define BT_UUID_NEW_NETWORK_COMMAND                              BT_UUID_DECLARE_128(BT_UUID_NEW_NETWORK_COMMAND_VAL)
#define BT_UUID_NEW_NETWORK_STATUS                               BT_UUID_DECLARE_128(BT_UUID_NEW_NETWORK_STATUS_VAL)

#define CMD_CREATE_NEW_NETWORK                                   0x01

LOG_MODULE_REGISTER(ble_create_network, LOG_LEVEL_INF);

static otOperationalDataset dataset;
static struct k_work create_new_work;

void create_new_network(struct k_work *work);

static ssize_t read_network_name(struct bt_conn *conn,
                                 const struct bt_gatt_attr *attr,
                                 void *buf, uint16_t len, 
                                 uint16_t offset) {
    uint8_t (*value)[17] = attr->user_data;
    LOG_INF("Network Name read");
    return bt_gatt_attr_read(conn, attr, buf, len, offset, value, sizeof(*value));
}

static ssize_t write_network_name(struct bt_conn *conn,
                                  const struct bt_gatt_attr *attr,
                                  const void *buf, uint16_t len,
                                  uint16_t offset, uint8_t flags) {
    if (len != 17U) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
    }

    if (offset != 0) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }

    memcpy(attr->user_data, buf, len);
    LOG_HEXDUMP_INF(dataset.mNetworkName.m8, sizeof(dataset.mNetworkName.m8), "Network Name written");
    return len;
}

static ssize_t read_network_key(struct bt_conn *conn,
                                 const struct bt_gatt_attr *attr,
                                 void *buf, uint16_t len, 
                                 uint16_t offset) {
    uint8_t (*value)[16] = attr->user_data;
    LOG_INF("Network Key read");
    return bt_gatt_attr_read(conn, attr, buf, len, offset, value, sizeof(*value));
}

static ssize_t read_ext_panid(struct bt_conn *conn,
                                 const struct bt_gatt_attr *attr,
                                 void *buf, uint16_t len, 
                                 uint16_t offset) {
    uint8_t (*value)[8] = attr->user_data;
    LOG_INF("ext panid read");
    return bt_gatt_attr_read(conn, attr, buf, len, offset, value, sizeof(*value));
}

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
        case CMD_CREATE_NEW_NETWORK:
            LOG_INF("COMMAND SET : CREATE NEW");
            k_work_submit_to_queue(get_commission_work_q(), &create_new_work);
            break;

        default:
            return BT_GATT_ERR(BT_ATT_ERR_NOT_SUPPORTED);
            break;
    }

    return len;
}

void init_ble_create_network() {
    k_work_init(&create_new_work, create_new_network);
}

void create_new_network(struct k_work *work) {
    LOG_INF("Let's Create New NETWORK");
    otOperationalDataset new_dataset;
    otError err;
    err = otDatasetCreateNewNetwork(openthread_get_default_instance(), &new_dataset);
    
    if (err == OT_ERROR_FAILED) {
        LOG_ERR("FAILED: Create NEW Network");
        return;
    }

    memcpy(&new_dataset.mNetworkName.m8, &dataset.mNetworkName.m8, 17);
    memcpy(&dataset, &new_dataset, sizeof(new_dataset));

    otDatasetSetActive(openthread_get_default_instance(), &dataset);

    LOG_INF("Done: Create New Network");

    openthread_start(openthread_get_default_context());
    LOG_INF("Done: Start New Network");
}

BT_GATT_SERVICE_DEFINE(
    thread_commission_service,
    BT_GATT_PRIMARY_SERVICE(BT_UUID_NEW_NETWORK_SERVICE),
    BT_GATT_CHARACTERISTIC(BT_UUID_NEW_NETWORK_NETWORK_NAME,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
                           BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
                           read_network_name, write_network_name, &(dataset.mNetworkName.m8)),
    BT_GATT_CHARACTERISTIC(BT_UUID_NEW_NETWORK_NETWORK_KEY,
                           BT_GATT_CHRC_READ,
                           BT_GATT_PERM_READ,
                           read_network_key, NULL, &(dataset.mNetworkKey.m8)),
    BT_GATT_CHARACTERISTIC(BT_UUID_NEW_NETWORK_EXT_PANID,
                           BT_GATT_CHRC_READ,
                           BT_GATT_PERM_READ,
                           read_ext_panid, NULL, &(dataset.mExtendedPanId.m8)),
    BT_GATT_CHARACTERISTIC(BT_UUID_NEW_NETWORK_COMMAND,
                           BT_GATT_CHRC_WRITE,
                           BT_GATT_PERM_WRITE,
                           NULL, write_command, NULL),
    BT_GATT_CHARACTERISTIC(BT_UUID_NEW_NETWORK_STATUS,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_INDICATE,
                           BT_GATT_PERM_READ,
                           NULL, NULL, NULL),
    BT_GATT_CCC(NULL, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);