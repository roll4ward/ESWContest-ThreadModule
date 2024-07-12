#ifndef BLE_COMMISSIONING_H
#define BLE_COMMISSIONING_H
#include <zephyr/bluetooth/gatt.h>
enum status {
    WAITING = (uint8_t)0x00,
    PROGRESSING,
    FAILED,
    DONE,
};
typedef uint8_t status;

enum commission_command {
    NEW_NETWORK = 0x01,
    JOIN_NETWORK = 0x02,
    RESET_DATASET = 0x03,
};
typedef uint8_t commission_command;

enum scan_command {
    SCAN = 0x01,
    RESET_QUEUE = 0x02,
    GET = 0x03,
};
typedef uint8_t scan_command;

typedef struct user_data_info {
    void * data;
    size_t len;
} user_data_info;

#define USER_DATA_INFO(_type, _name, _len, _init_val) \
static _type _name##_data = _init_val; \
static struct user_data_info _name = { \
    .data = &(_name##_data), \
    .len = _len \
}

#define USER_DATA_ORIGIN(_name) (_name##_data)
#define USER_DATA(_name) _name##.data
#define USER_DATA_REF(_name) CONTAINER_OF(_name)
#define USER_DATA_LENGTH(_name) sizeof(_name##_data)

ssize_t read_gatt(struct bt_conn *conn,
                                 const struct bt_gatt_attr *attr,
                                 void *buf, uint16_t len, 
                                 uint16_t offset);

void init_ble_commission();
#endif