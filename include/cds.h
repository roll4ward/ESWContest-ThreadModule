#ifndef ROLL4_CDS_H
#define ROLL4_CDS_H

#include "openthread/coap.h"

int init_cds();
double get_cds_value();
otCoapResource *get_cds_resource();
#endif