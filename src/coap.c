#include "coap.h"
#include "util.h"
#include "zephyr/net/openthread.h"
#include "zephyr/logging/log.h"

LOG_MODULE_REGISTER(coap, LOG_LEVEL_INF);

static void user_data_handler(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo) {
    otError error;
    UserData *data = (UserData *)aContext;
    otMessage *responseMessage = NULL;
    
    responseMessage = otCoapNewMessage(openthread_get_default_instance(), NULL);
    EXPECT_OR_EXIT(responseMessage != NULL);
    LOG_INF("Message Created");
    
    EXPECT_NO_ERROR_OR_EXIT((error = otCoapMessageInitResponse(responseMessage, aMessage, OT_COAP_TYPE_NON_CONFIRMABLE, OT_COAP_CODE_VALID)));
    LOG_INF("Message initialized as respose");
    EXPECT_NO_ERROR_OR_EXIT((error = otCoapMessageSetPayloadMarker(responseMessage)));
    LOG_INF("Set Payload Marker");
    
    data->mUpdateHandler(data);
    LOG_INF("USER Data Updated");

    EXPECT_NO_ERROR_OR_EXIT((error = otMessageAppend(responseMessage, data->mUserData, data->len)));
    LOG_INF("Payload Appended");
    EXPECT_NO_ERROR_OR_EXIT((error = otCoapSendResponse(openthread_get_default_instance(), responseMessage, aMessageInfo)));
    LOG_INF("Message sent");

exit:
    if (error != OT_ERROR_NONE && responseMessage != NULL) {
        LOG_INF("Free Message");
        otMessageFree(responseMessage);
    }
    LOG_INF("Exit handler");
}

void addCoAPResource(otCoapResource *aResource) {
    aResource->mHandler = user_data_handler;
    otCoapAddResource(openthread_get_default_instance(), aResource);
};