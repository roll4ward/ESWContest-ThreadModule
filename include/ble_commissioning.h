#ifndef BLE_COMMISSIONING_H
#define BLE_COMMISSIONING_H

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

void init_ble_commission();
#endif