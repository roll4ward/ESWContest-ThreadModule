#ifdef CONFIG_ROLL4_PUMP
#ifndef ROLL4_PUMP_H
#define ROLL4_PUMP_H
#include "openthread/coap.h"

int set_pump_value(double value);
otCoapResource *get_pump_resource();
#endif
#endif