#ifndef ROLL4_SOIL_HUMIDITY_H
#define ROLL4_SOIL_HUMIDITY_H

#include "openthread/coap.h"

int init_soil_humidity();
double get_soil_humidity_value();
otCoapResource *get_soil_humidity_resource();
#endif