#ifndef ROLL4_COAP_H
#define ROLL4_COAP_H

#include "openthread/coap.h"

#define DEFINE_COAP_RESOURCE(uri, user_data) otCoapResource uri##_resource = {\
                                            .mUriPath = #uri,\
                                            .mContext = user_data };

typedef struct UserData UserData;
typedef void (*UpdateHandler)(UserData *aUserData);

struct UserData {
    void *mUserData;
    size_t len;
    UpdateHandler mUpdateHandler;
};

void addCoAPResource(otCoapResource *aResource);
#endif