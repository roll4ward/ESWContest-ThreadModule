#ifndef ROLL4_TEMPERATURE_HUMIDITY_H
#define ROLL4_TEMPERATURE_HUMIDITY_H

#include "openthread/coap.h"

double get_temperature_value();
double get_humidity_value();
otCoapResource *get_temperature_resource();
otCoapResource *get_humidity_resource();
#endif