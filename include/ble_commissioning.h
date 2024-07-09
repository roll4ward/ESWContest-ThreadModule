#ifndef BLE_COMMISSIONING_H
#define BLE_COMMISSIONING_H

#define STATE_WAITING                                            0X00
#define STATE_PROGRESSING                                        0X01
#define STATE_FAILED                                             0X02
#define STATE_DONE                                               0X03

void init_ble_commission();
#endif