#ifndef ROLL4_COAP_H
#define ROLL4_COAP_H

#include "openthread/coap.h"

#define DEFINE_COAP_RESOURCE(uri, user_data) otCoapResource uri##_resource = {\
                                            .mUriPath = #uri,\
                                            .mContext = user_data };

typedef void (*UpdateHandler)(void *aUserData);

typedef struct UserData {
    void *mUserData;
    size_t len;
    UpdateHandler mUpdateHandler;
} UserData;

void addCoAPResource(otCoapResource aResource);
#endif