#include "ble_commissioning.h"
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ble_thread_common, LOG_LEVEL_INF);

ssize_t read_gatt(struct bt_conn *conn,
                                 const struct bt_gatt_attr *attr,
                                 void *buf, uint16_t len, 
                                 uint16_t offset) {
    user_data_info *info = (user_data_info *)attr->user_data;
    LOG_HEXDUMP_DBG(((struct bt_uuid_128 *)BT_UUID_128(attr->uuid))->val, 16U, "Data read. UUID :");
    return bt_gatt_attr_read(conn, attr, buf, len, offset, info->data, info->len);                                
}

ssize_t write_gatt(struct bt_conn *conn,
                                  const struct bt_gatt_attr *attr,
                                  const void *buf, uint16_t len,
                                  uint16_t offset, uint8_t flags) {
    user_data_info *info = (user_data_info *)attr->user_data;
    if (len != info->len) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
    }

    if (offset != 0) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }

    memcpy(info->data, buf, len);
    LOG_HEXDUMP_DBG(((struct bt_uuid_128 *)BT_UUID_128(attr->uuid))->val, 16U, "Data Write. UUID :");
    LOG_HEXDUMP_DBG(info->data, info->len, "Value :");
    return len;
}