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

#define BT_UUID_COMMISSION_SERVICE_VAL                           BT_UUID_128_ENCODE(0x9fff0001,0x89f6, 0x4f1b, 0x9e5d, 0x0d4648d944c9)
#define BT_UUID_COMMISSION_NETWORK_NAME_VAL                      BT_UUID_128_ENCODE(0x9fff0002,0x89f6, 0x4f1b, 0x9e5d, 0x0d4648d944c9)
#define BT_UUID_COMMISSION_NETWORK_KEY_VAL                       BT_UUID_128_ENCODE(0x9fff0003,0x89f6, 0x4f1b, 0x9e5d, 0x0d4648d944c9)
#define BT_UUID_COMMISSION_EXT_PANID_VAL                         BT_UUID_128_ENCODE(0x9fff0004,0x89f6, 0x4f1b, 0x9e5d, 0x0d4648d944c9)
#define BT_UUID_COMMISSION_COMMAND_VAL                           BT_UUID_128_ENCODE(0x9fff0005,0x89f6, 0x4f1b, 0x9e5d, 0x0d4648d944c9)
#define BT_UUID_COMMISSION_STATUS_VAL                            BT_UUID_128_ENCODE(0x9fff0006,0x89f6, 0x4f1b, 0x9e5d, 0x0d4648d944c9)
#define BT_UUID_COMMISSION_ROLE_VAL                              BT_UUID_128_ENCODE(0x9fff0007,0x89f6, 0x4f1b, 0x9e5d, 0x0d4648d944c9)

#define BT_UUID_COMMISSION_SERVICE                               BT_UUID_DECLARE_128(BT_UUID_COMMISSION_SERVICE_VAL)
#define BT_UUID_COMMISSION_NETWORK_NAME                          BT_UUID_DECLARE_128(BT_UUID_COMMISSION_NETWORK_NAME_VAL)
#define BT_UUID_COMMISSION_NETWORK_KEY                           BT_UUID_DECLARE_128(BT_UUID_COMMISSION_NETWORK_KEY_VAL)
#define BT_UUID_COMMISSION_EXT_PANID                             BT_UUID_DECLARE_128(BT_UUID_COMMISSION_EXT_PANID_VAL)
#define BT_UUID_COMMISSION_COMMAND                               BT_UUID_DECLARE_128(BT_UUID_COMMISSION_COMMAND_VAL)
#define BT_UUID_COMMISSION_STATUS                                BT_UUID_DECLARE_128(BT_UUID_COMMISSION_STATUS_VAL)
#define BT_UUID_COMMISSION_ROLE                                  BT_UUID_DECLARE_128(BT_UUID_COMMISSION_ROLE_VAL)

LOG_MODULE_REGISTER(ble_create_network, LOG_LEVEL_DBG);

// User Data Section

// User Data Info
USER_DATA_INFO(otNetworkName, networkname, OT_NETWORK_NAME_MAX_SIZE + 1, {0});
USER_DATA_INFO(otNetworkKey, networkkey, OT_NETWORK_KEY_SIZE, {0});
USER_DATA_INFO(otExtendedPanId, extpanid, OT_EXT_PAN_ID_SIZE, {0});
USER_DATA_INFO(status, commission_status, 1U, WAITING);
USER_DATA_INFO(otDeviceRole, role, 1U, OT_DEVICE_ROLE_DISABLED);

// Indicate Enable
static bool status_indicate_enabled = false;
static bool role_indicate_enabled = false;

// Indicate Parameters
static struct bt_gatt_indicate_params status_ind_params = {
    .uuid = BT_UUID_COMMISSION_STATUS,
    .func = NULL,
    .destroy = NULL,
    .data = &USER_DATA(commission_status),
    .len = USER_DATA_LENGTH(commission_status)
};

static struct bt_gatt_indicate_params role_ind_params = {
    .uuid = BT_UUID_COMMISSION_ROLE,
    .func = NULL,
    .destroy = NULL,
    .data = &USER_DATA(role),
    .len = USER_DATA_LENGTH(role)
};

static void create_new_network(struct k_work *work);
static void join_network(struct k_work *work);
static void reset_dataset(struct k_work *work);

K_WORK_DEFINE(create_new_work, create_new_network);
K_WORK_DEFINE(join_work, join_network);
K_WORK_DEFINE(reset_dataset_work, reset_dataset);

static int commission_state_indicate(uint8_t state);

ssize_t read_network_name(struct bt_conn *conn,
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
    LOG_HEXDUMP_DBG(&USER_DATA(networkname), sizeof(otNetworkName), "Network Name written");
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
    LOG_HEXDUMP_DBG(&USER_DATA(networkkey), sizeof(otNetworkKey), "Network Key written");
    return len;
}

ssize_t read_ext_panid(struct bt_conn *conn,
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
    LOG_HEXDUMP_DBG(&USER_DATA(extpanid), sizeof(otExtendedPanId), "Extended panid written");
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
        case NEW_NETWORK:
            LOG_DBG("COMMAND SET : CREATE NEW");
            k_work_submit(&create_new_work);
            break;

        case JOIN_NETWORK:
            LOG_DBG("COMMAND SET : JOIN");
            k_work_submit(&join_work);
            break;

        case RESET_DATASET:
            LOG_DBG("COMMAND SET : RESET");
            k_work_submit(&reset_dataset_work);
            break;

        default:
            return BT_GATT_ERR(BT_ATT_ERR_NOT_SUPPORTED);
            break;
    }

    return len;
}

ssize_t read_status(struct bt_conn *conn,
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
                           read_network_name, write_network_name, &USER_DATA(networkname)),
    BT_GATT_CHARACTERISTIC(BT_UUID_COMMISSION_NETWORK_KEY,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
                           BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
                           read_network_key, write_network_key, &USER_DATA(networkkey)),
    BT_GATT_CHARACTERISTIC(BT_UUID_COMMISSION_EXT_PANID,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
                           BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
                           read_ext_panid, write_ext_panid, &USER_DATA(extpanid)),
    BT_GATT_CHARACTERISTIC(BT_UUID_COMMISSION_COMMAND,
                           BT_GATT_CHRC_WRITE,
                           BT_GATT_PERM_WRITE,
                           NULL, write_command, NULL),
    BT_GATT_CHARACTERISTIC(BT_UUID_COMMISSION_STATUS,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_INDICATE,
                           BT_GATT_PERM_READ,
                           read_status, NULL, &USER_DATA(commission_status)),
    BT_GATT_CCC(status_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),                      
    BT_GATT_CHARACTERISTIC(BT_UUID_COMMISSION_ROLE,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_INDICATE,
                           BT_GATT_PERM_READ,
                           read_role, NULL, &USER_DATA(role)),
    BT_GATT_CCC(role_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

static void create_new_network(struct k_work *work) {
    otOperationalDataset new_dataset;
    otError err;
    
    openthread_api_mutex_lock(openthread_get_default_context());
    otIp6SetEnabled(openthread_get_default_instance(), true);
    otThreadSetEnabled(openthread_get_default_instance(), false);

    LOG_DBG("Let's Create New NETWORK");
    commission_state_indicate(PROGRESSING);

    err = otDatasetCreateNewNetwork(openthread_get_default_instance(), &new_dataset);
    
    if (err != OT_ERROR_NONE) {
        LOG_ERR("FAILED: Create NEW Network");
        commission_state_indicate(FAILED);
        return;
    }

    memcpy(&new_dataset.mNetworkName, &USER_DATA(networkname), sizeof(otNetworkName));
    memcpy(&USER_DATA(networkkey), &new_dataset.mNetworkKey, sizeof(otNetworkKey));
    memcpy(&USER_DATA(extpanid), &new_dataset.mExtendedPanId, sizeof(otExtendedPanId));

    err = otDatasetSetActive(openthread_get_default_instance(), &new_dataset);

    if (err != OT_ERROR_NONE) {
        LOG_ERR("FAILED: Create NEW Network");
        commission_state_indicate(FAILED);
        return;
    }

    LOG_DBG("Done: Create New Network");
    otThreadSetEnabled(openthread_get_default_instance(), true);
    openthread_api_mutex_unlock(openthread_get_default_context());

    commission_state_indicate(DONE);
}

static void join_network(struct k_work *work) {\
    otError err;
    otOperationalDataset join_dataset = {0};
    
    openthread_api_mutex_lock(openthread_get_default_context());
    otIp6SetEnabled(openthread_get_default_instance(), true);
    otThreadSetEnabled(openthread_get_default_instance(), false);

    LOG_DBG("Let's Join New NETWORK");
    commission_state_indicate(PROGRESSING);

    memcpy(&join_dataset.mNetworkName, &USER_DATA(networkname), sizeof(otNetworkName));
    join_dataset.mComponents.mIsNetworkNamePresent = true;
    memcpy(&join_dataset.mNetworkKey, &USER_DATA(networkkey), sizeof(otNetworkKey));
    join_dataset.mComponents.mIsNetworkKeyPresent = true;
    memcpy(&join_dataset.mExtendedPanId, &USER_DATA(extpanid), sizeof(otExtendedPanId));
    join_dataset.mComponents.mIsExtendedPanIdPresent = true;

    err = otDatasetSetActive(openthread_get_default_instance(), &join_dataset);
    if (err != OT_ERROR_NONE) {
        LOG_ERR("cannot update DATASET");
        return;
    }

    LOG_DBG("Done: Join New Network");

    otThreadSetEnabled(openthread_get_default_instance(), true);
    openthread_api_mutex_unlock(openthread_get_default_context());

    commission_state_indicate(DONE);
}

static void reset_dataset(struct k_work *work) {
    commission_state_indicate(PROGRESSING);

    memset(&USER_DATA(networkname), 0, sizeof(otNetworkName));
    memset(&USER_DATA(networkkey), 0, sizeof(otNetworkKey));
    memset(&USER_DATA(extpanid), 0, sizeof(otExtendedPanId));
    
    commission_state_indicate(DONE);
}

static int commission_state_indicate(uint8_t state) {
    USER_DATA(commission_status) = state;

    if (!status_indicate_enabled) {
		return -EACCES;
	}

	return bt_gatt_indicate(NULL, &status_ind_params);
}

static int role_indicate() {
    USER_DATA(role) = otThreadGetDeviceRole(openthread_get_default_instance());
    LOG_DBG("role : %d", USER_DATA(role));

    if (!role_indicate_enabled) {
		return -EACCES;
	}

	return bt_gatt_indicate(NULL, &role_ind_params);
}

static void state_changed_cb(otChangedFlags flag, void *context) {
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