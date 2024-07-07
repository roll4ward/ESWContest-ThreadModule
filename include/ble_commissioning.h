#ifndef BLE_COMMISSIONING_H
#define BLE_COMMISSIONING_H
#include <zephyr/kernel.h>

void init_ble_commissioning_service();
struct k_work_q *get_commission_work_q();

void init_ble_create_network();
#endif