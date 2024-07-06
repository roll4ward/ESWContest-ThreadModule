#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

#include <openthread/dataset.h>

#include "ble_commissioning.h"

#define BT_UUID_TNCS_VAL                                         BT_UUID_128_ENCODE(0x9fff0001,0x89f6, 0x4f1b, 0x9e5d, 0xd4648d944c9)
#define BT_UUID_TNCS_NETWORK_NAME_VAL                            BT_UUID_128_ENCODE(0x9fff0002,0x89f6, 0x4f1b, 0x9e5d, 0xd4648d944c9)
#define BT_UUID_TNCS_NETWORK_KEY_VAL                             BT_UUID_128_ENCODE(0x9fff0003,0x89f6, 0x4f1b, 0x9e5d, 0xd4648d944c9)
#define BT_UUID_TNCS_EXT_PANID_VAL                               BT_UUID_128_ENCODE(0x9fff0004,0x89f6, 0x4f1b, 0x9e5d, 0xd4648d944c9)
#define BT_UUID_TNCS_COMMAND_VAL                                 BT_UUID_128_ENCODE(0x9fff0005,0x89f6, 0x4f1b, 0x9e5d, 0xd4648d944c9)
#define BT_UUID_TNCS_STATUS_VAL                                  BT_UUID_128_ENCODE(0x9fff0006,0x89f6, 0x4f1b, 0x9e5d, 0xd4648d944c9)

#define BT_UUID_TNCS                                             BT_UUID_DECLARE_128(BT_UUID_TNCS_VAL)
#define BT_UUID_TNCS_NETWORK_NAME                                BT_UUID_DECLARE_128(BT_UUID_TNCS_NETWORK_NAME_VAL)
#define BT_UUID_TNCS_NETWORK_KEY                                 BT_UUID_DECLARE_128(BT_UUID_TNCS_NETWORK_KEY_VAL)
#define BT_UUID_TNCS_EXT_PANID                                   BT_UUID_DECLARE_128(BT_UUID_TNCS_EXT_PANID_VAL)
#define BT_UUID_TNCS_COMMAND                                     BT_UUID_DECLARE_128(BT_UUID_TNCS_COMMAND_VAL)
#define BT_UUID_TNCS_STATUS                                      BT_UUID_DECLARE_128(BT_UUID_TNCS_STATUS_VAL)

#define CMD_CREATE_NEW_NETWORK                                   0x01
#define CMD_JOIN_NETWORK                                         0x02

#define COMMISSION_QUEUE_STACK_SIZE                              512
#define COMMISSION_WORKQUEUE_PRIORITY                            3

LOG_MODULE_REGISTER(ble_commission, LOG_LEVEL_INF);

static struct k_work_q commission_work_q;
K_THREAD_STACK_DEFINE(commission_work_stack, COMMISSION_QUEUE_STACK_SIZE);

static struct k_work create_new_work;
static struct k_work join_work;

void create_new_network(struct k_work *work);
void join_network(struct k_work *work);

otOperationalDataset dataset;

void init_ble_commissioning_service() {
    k_work_init(&create_new_work, create_new_network);
    k_work_init(&join_work, join_network);

    k_work_queue_init(&commission_work_q);
    k_work_queue_start(&commission_work_q, commission_work_stack,
                        K_THREAD_STACK_SIZEOF(commission_work_stack),
                        COMMISSION_WORKQUEUE_PRIORITY, NULL);
}

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

static ssize_t write_network_key(struct bt_conn *conn,
                                  const struct bt_gatt_attr *attr,
                                  const void *buf, uint16_t len,
                                  uint16_t offset, uint8_t flags) {
    if (len != 16U) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
    }

    if (offset != 0) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }
    memcpy(attr->user_data, buf, len);
    LOG_HEXDUMP_INF(dataset.mNetworkKey.m8, sizeof(dataset.mNetworkKey.m8), "Network key written");
    return len;
}

static ssize_t read_ext_panid(struct bt_conn *conn,
                                 const struct bt_gatt_attr *attr,
                                 void *buf, uint16_t len, 
                                 uint16_t offset) {
    uint8_t (*value)[8] = attr->user_data;
    LOG_INF("ext panid read");
    return bt_gatt_attr_read(conn, attr, buf, len, offset, value, sizeof(*value));
}

static ssize_t write_ext_panid(struct bt_conn *conn,
                                  const struct bt_gatt_attr *attr,
                                  const void *buf, uint16_t len,
                                  uint16_t offset, uint8_t flags) {
    if (len != 8U) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
    }

    if (offset != 0) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }

    memcpy(attr->user_data, buf, len);
    LOG_HEXDUMP_INF(dataset.mExtendedPanId.m8, sizeof(dataset.mExtendedPanId.m8), "Expanded PANID written");
    return len;
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
            k_work_submit(&create_new_work);
            break;

        case CMD_JOIN_NETWORK:
            LOG_INF("COMMAND SET : JOIN");
            k_work_submit(&join_work);
            break;

        default:
            return BT_GATT_ERR(BT_ATT_ERR_NOT_SUPPORTED);
            break;
    }

    return len;
}

void create_new_network(struct k_work *work) {
    LOG_INF("Let's Create New NETWORK");
}
void join_network(struct k_work *work) {
    LOG_INF("Let's join NETWORK");
}

BT_GATT_SERVICE_DEFINE(
    thread_commission_service,
    BT_GATT_PRIMARY_SERVICE(BT_UUID_TNCS),
    BT_GATT_CHARACTERISTIC(BT_UUID_TNCS_NETWORK_NAME,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
                           BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
                           read_network_name, write_network_name, &(dataset.mNetworkName.m8)),
    BT_GATT_CHARACTERISTIC(BT_UUID_TNCS_NETWORK_KEY,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
                           BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
                           read_network_key, write_network_key, &(dataset.mNetworkKey.m8)),
    BT_GATT_CHARACTERISTIC(BT_UUID_TNCS_EXT_PANID,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
                           BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
                           read_ext_panid, write_ext_panid, &(dataset.mExtendedPanId.m8)),
    BT_GATT_CHARACTERISTIC(BT_UUID_TNCS_COMMAND,
                           BT_GATT_CHRC_WRITE,
                           BT_GATT_PERM_WRITE,
                           NULL, write_command, NULL),
    BT_GATT_CHARACTERISTIC(BT_UUID_TNCS_STATUS,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_INDICATE,
                           BT_GATT_PERM_READ,
                           NULL, NULL, NULL),
    BT_GATT_CCC(NULL, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);