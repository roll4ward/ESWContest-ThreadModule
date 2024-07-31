#ifndef BLE_COMMISSIONING_H
#define BLE_COMMISSIONING_H
#include <zephyr/bluetooth/gatt.h>
enum status {
    WAITING = 0x00,
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

#define USER_DATA(_name) (_name##_data)
#define USER_DATA_LENGTH(_name) sizeof(_name##_data)


#define INDICATE_DEFINE(_name, _uuid) \
        static bool _name##_indicate_enabled = false; \
        static void _name##_ccc_cfg_changed(const struct bt_gatt_attr *attr, \
                                            uint16_t value) { \
                _name##_indicate_enabled = (value == BT_GATT_CCC_INDICATE);} \
        struct bt_gatt_indicate_params _name##ind_params = {\
                .uuid = _uuid ,\
            }; \
        static int _name##_indicate() {\
            if (!_name##_indicate_enabled) return -EACCES; \
            _name##ind_params.data = _name.data;\
            _name##ind_params.len = _name.len;\
            return bt_gatt_indicate(NULL, &_name##ind_params);\
        }
#define INDICATE(_name) _name##_indicate();
#define INDICATE_VALUE(_name, _value) USER_DATA(_name) = _value; \
                                      INDICATE(_name);
#define INDICATE_CCC_CALLBACK(_name) _name##_ccc_cfg_changed


#define COMMAND_WORK_DECLARE(_name) \
            static void _name(struct k_work *work); \
            K_WORK_DEFINE(_name##_work, _name);

#define COMMAND_WORK_HANDLER(_name) \
            static void _name(struct k_work *work)

#define COMMAND_WORK_NAME(_name) _name##_work

ssize_t read_gatt(struct bt_conn *conn,
                                 const struct bt_gatt_attr *attr,
                                 void *buf, uint16_t len, 
                                 uint16_t offset);

ssize_t write_gatt(struct bt_conn *conn,
                                  const struct bt_gatt_attr *attr,
                                  const void *buf, uint16_t len,
                                  uint16_t offset, uint8_t flags);                                 

void init_ble_commission();
#endif