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

#define BT_UUID_COMMISSION_SERVICE_VAL                           BT_UUID_128_ENCODE(0x9fff0001,0x89f6, 0x4f1b, 0x9e5d, 0xd4648d944c9)
#define BT_UUID_COMMISSION_NETWORK_NAME_VAL                      BT_UUID_128_ENCODE(0x9fff0002,0x89f6, 0x4f1b, 0x9e5d, 0xd4648d944c9)
#define BT_UUID_COMMISSION_NETWORK_KEY_VAL                       BT_UUID_128_ENCODE(0x9fff0003,0x89f6, 0x4f1b, 0x9e5d, 0xd4648d944c9)
#define BT_UUID_COMMISSION_EXT_PANID_VAL                         BT_UUID_128_ENCODE(0x9fff0004,0x89f6, 0x4f1b, 0x9e5d, 0xd4648d944c9)
#define BT_UUID_COMMISSION_COMMAND_VAL                           BT_UUID_128_ENCODE(0x9fff0005,0x89f6, 0x4f1b, 0x9e5d, 0xd4648d944c9)
#define BT_UUID_COMMISSION_STATUS_VAL                            BT_UUID_128_ENCODE(0x9fff0006,0x89f6, 0x4f1b, 0x9e5d, 0xd4648d944c9)
#define BT_UUID_COMMISSION_ROLE_VAL                              BT_UUID_128_ENCODE(0x9fff0007,0x89f6, 0x4f1b, 0x9e5d, 0xd4648d944c9)

#define BT_UUID_COMMISSION_SERVICE                              BT_UUID_DECLARE_128(BT_UUID_COMMISSION_SERVICE_VAL)
#define BT_UUID_COMMISSION_NETWORK_NAME                         BT_UUID_DECLARE_128(BT_UUID_COMMISSION_NETWORK_NAME_VAL)
#define BT_UUID_COMMISSION_NETWORK_KEY                          BT_UUID_DECLARE_128(BT_UUID_COMMISSION_NETWORK_KEY_VAL)
#define BT_UUID_COMMISSION_EXT_PANID                            BT_UUID_DECLARE_128(BT_UUID_COMMISSION_EXT_PANID_VAL)
#define BT_UUID_COMMISSION_COMMAND                              BT_UUID_DECLARE_128(BT_UUID_COMMISSION_COMMAND_VAL)
#define BT_UUID_COMMISSION_STATUS                               BT_UUID_DECLARE_128(BT_UUID_COMMISSION_STATUS_VAL)
#define BT_UUID_COMMISSION_ROLE                                 BT_UUID_DECLARE_128(BT_UUID_COMMISSION_ROLE_VAL)

#define CMD_CREATE_NEW_NETWORK                                   0x01
#define CMD_JOIN_NETWORK                                         0X02
#define CMD_RESET_DATASET                                        0X03
#define CMD_THREAD_START                                         0x04

#define STATE_WAITING                                            0X00
#define STATE_PROGRESSING                                        0X01
#define STATE_FAILED                                             0X02
#define STATE_DONE                                               0X03


LOG_MODULE_REGISTER(ble_create_network, LOG_LEVEL_INF);

static otOperationalDataset dataset;
static uint8_t commission_state = STATE_WAITING;
static uint8_t role = OT_DEVICE_ROLE_DISABLED;

static bool status_indicate_enabled = false;
static bool role_indicate_enabled = false;

static struct bt_gatt_indicate_params status_ind_params = {
    .uuid = BT_UUID_COMMISSION_STATUS,
    .func = NULL,
    .destroy = NULL,
    .data = &commission_state,
    .len = sizeof(commission_state)
};

static struct bt_gatt_indicate_params role_ind_params = {
    .uuid = BT_UUID_COMMISSION_ROLE,
    .func = NULL,
    .destroy = NULL,
    .data = &role,
    .len = sizeof(role)
};

static void create_new_network(struct k_work *work);
static void join_network(struct k_work *work);
static void reset_dataset(struct k_work *work);
static void start_thread(struct k_work *work);

K_WORK_DEFINE(create_new_work, create_new_network);
K_WORK_DEFINE(join_work, join_network);
K_WORK_DEFINE(reset_dataset_work, reset_dataset);
K_WORK_DEFINE(start_thread_work, start_thread);

static int commission_state_indicate(uint8_t state);

static ssize_t read_network_name(struct bt_conn *conn,
                                 const struct bt_gatt_attr *attr,
                                 void *buf, uint16_t len, 
                                 uint16_t offset) {
    uint8_t (*value)[17] = attr->user_data;
    LOG_DBG("Network Name read");
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
    LOG_HEXDUMP_DBG(dataset.mNetworkName.m8, sizeof(dataset.mNetworkName.m8), "Network Name written");
    return len;
}

static ssize_t read_network_key(struct bt_conn *conn,
                                 const struct bt_gatt_attr *attr,
                                 void *buf, uint16_t len, 
                                 uint16_t offset) {
    uint8_t (*value)[16] = attr->user_data;
    LOG_DBG("Network Key read");
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
    LOG_HEXDUMP_DBG(dataset.mNetworkKey.m8, sizeof(dataset.mNetworkKey.m8), "Network Key written");
    return len;
}

static ssize_t read_ext_panid(struct bt_conn *conn,
                                 const struct bt_gatt_attr *attr,
                                 void *buf, uint16_t len, 
                                 uint16_t offset) {
    uint8_t (*value)[8] = attr->user_data;
    LOG_DBG("ext panid read");
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
    LOG_HEXDUMP_DBG(dataset.mExtendedPanId.m8, sizeof(dataset.mExtendedPanId.m8), "Extended panid written");
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
            LOG_DBG("COMMAND SET : CREATE NEW");
            k_work_submit_to_queue(get_commission_work_q(), &create_new_work);
            break;

        case CMD_JOIN_NETWORK:
            LOG_DBG("COMMAND SET : JOIN");
            k_work_submit_to_queue(get_commission_work_q(), &join_work);
            break;

        case CMD_RESET_DATASET:
            LOG_DBG("COMMAND SET : RESET");
            k_work_submit_to_queue(get_commission_work_q(), &reset_dataset_work);
            break;

        case CMD_THREAD_START:
            LOG_DBG("COMMAND SET : THREAD START");
            k_work_submit_to_queue(get_commission_work_q(), &start_thread_work);
            break;

        default:
            return BT_GATT_ERR(BT_ATT_ERR_NOT_SUPPORTED);
            break;
    }

    return len;
}

static ssize_t read_status(struct bt_conn *conn,
                                 const struct bt_gatt_attr *attr,
                                 void *buf, uint16_t len, 
                                 uint16_t offset) {
    uint8_t *value = attr->user_data;
    LOG_DBG("state read");
    return bt_gatt_attr_read(conn, attr, buf, len, offset, value, sizeof(*value));
}

static ssize_t read_role(struct bt_conn *conn,
                                 const struct bt_gatt_attr *attr,
                                 void *buf, uint16_t len, 
                                 uint16_t offset) {
    uint8_t *value = attr->user_data;
    LOG_DBG("role read");
    return bt_gatt_attr_read(conn, attr, buf, len, offset, value, sizeof(*value));
}

static void status_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value) {
    status_indicate_enabled = (value == BT_GATT_CCC_INDICATE);
}

static void role_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value) {
    role_indicate_enabled = (value == BT_GATT_CCC_INDICATE);
}

BT_GATT_SERVICE_DEFINE(
    thread_commission_service,
    BT_GATT_PRIMARY_SERVICE(BT_UUID_COMMISSION_SERVICE),
    BT_GATT_CHARACTERISTIC(BT_UUID_COMMISSION_NETWORK_NAME,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
                           BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
                           read_network_name, write_network_name, &(dataset.mNetworkName.m8)),
    BT_GATT_CHARACTERISTIC(BT_UUID_COMMISSION_NETWORK_KEY,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
                           BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
                           read_network_key, write_network_key, &(dataset.mNetworkKey.m8)),
    BT_GATT_CHARACTERISTIC(BT_UUID_COMMISSION_EXT_PANID,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
                           BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
                           read_ext_panid, write_ext_panid, &(dataset.mExtendedPanId.m8)),
    BT_GATT_CHARACTERISTIC(BT_UUID_COMMISSION_COMMAND,
                           BT_GATT_CHRC_WRITE,
                           BT_GATT_PERM_WRITE,
                           NULL, write_command, NULL),
    BT_GATT_CHARACTERISTIC(BT_UUID_COMMISSION_STATUS,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_INDICATE,
                           BT_GATT_PERM_READ,
                           read_status, NULL, &commission_state),
    BT_GATT_CCC(status_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
    BT_GATT_CHARACTERISTIC(BT_UUID_COMMISSION_ROLE,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_INDICATE,
                           BT_GATT_PERM_READ,
                           read_role, NULL, &role),
    BT_GATT_CCC(role_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

static void create_new_network(struct k_work *work) {
    otOperationalDataset new_dataset;
    otError err;
    
    openthread_api_mutex_lock(openthread_get_default_context());
    otIp6SetEnabled(openthread_get_default_instance(), true);
    otThreadSetEnabled(openthread_get_default_instance(), false);

    LOG_DBG("Let's Create New NETWORK");
    commission_state_indicate(STATE_PROGRESSING);

    err = otDatasetCreateNewNetwork(openthread_get_default_instance(), &new_dataset);
    
    if (err != OT_ERROR_NONE) {
        LOG_ERR("FAILED: Create NEW Network");
        commission_state_indicate(STATE_FAILED);
        return;
    }

    memcpy(&new_dataset.mNetworkName.m8, &dataset.mNetworkName.m8, 17);
    memcpy(&dataset, &new_dataset, sizeof(new_dataset));

    err = otDatasetSetActive(openthread_get_default_instance(), &dataset);

    if (err != OT_ERROR_NONE) {
        LOG_ERR("FAILED: Create NEW Network");
        commission_state_indicate(STATE_FAILED);
        return;
    }

    LOG_DBG("Done: Create New Network");
    openthread_api_mutex_unlock(openthread_get_default_context());

    commission_state_indicate(STATE_DONE);
}

static void join_network(struct k_work *work) {\
    otOperationalDataset join_dataset = {0};
    otError err;
    
    openthread_api_mutex_lock(openthread_get_default_context());
    otIp6SetEnabled(openthread_get_default_instance(), true);
    otThreadSetEnabled(openthread_get_default_instance(), false);

    LOG_DBG("Let's Join New NETWORK");
    commission_state_indicate(STATE_PROGRESSING);

    memcpy(&join_dataset, &dataset, sizeof(dataset));

    LOG_HEXDUMP_INF(join_dataset.mNetworkKey.m8, sizeof(join_dataset.mNetworkKey.m8), "join network key");
    LOG_HEXDUMP_INF(join_dataset.mNetworkName.m8, sizeof(join_dataset.mNetworkName.m8), "join network name");
    LOG_HEXDUMP_INF(join_dataset.mExtendedPanId.m8, sizeof(join_dataset.mExtendedPanId.m8), "join ext panid");

    err = otDatasetSetActive(openthread_get_default_instance(), &join_dataset);

    if (err != OT_ERROR_NONE) {
        LOG_ERR("FAILED: Join NEW Network");
        commission_state_indicate(STATE_FAILED);
        return;
    }

    LOG_DBG("Done: Join New Network");

    openthread_api_mutex_unlock(openthread_get_default_context());

    commission_state_indicate(STATE_DONE);
}

static void reset_dataset(struct k_work *work) {
    commission_state_indicate(STATE_PROGRESSING);

    memset(&dataset, 0, sizeof(dataset));
    
    commission_state_indicate(STATE_DONE);
}

static void start_thread(struct k_work *work) {
    commission_state_indicate(STATE_PROGRESSING);
    openthread_api_mutex_lock(openthread_get_default_context());

    otThreadSetEnabled(openthread_get_default_instance(), true);

    openthread_api_mutex_unlock(openthread_get_default_context());
    commission_state_indicate(STATE_DONE);
}

static int commission_state_indicate(uint8_t state) {
    commission_state = state;

    if (!status_indicate_enabled) {
		return -EACCES;
	}

	return bt_gatt_indicate(NULL, &status_ind_params);
}

static int role_indicate() {
    role = otThreadGetDeviceRole(openthread_get_default_instance());
    LOG_INF("rold : %d", role);

    if (!role_indicate_enabled) {
		return -EACCES;
	}

	return bt_gatt_indicate(NULL, &role_ind_params);
}

static void *state_changed_cb(otChangedFlags flag, void *context) {
    if (flag & OT_CHANGED_THREAD_ROLE) {
        role_indicate();
    }
}

void init_ble_commission() {
    if (otSetStateChangedCallback(openthread_get_default_instance(), state_changed_cb, NULL) != OT_ERROR_NONE) {
        LOG_ERR("CALL BACK Register failed");
        return;
    }
}