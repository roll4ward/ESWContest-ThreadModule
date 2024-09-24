#include "coap.h"
#include "util.h"
#include "zephyr/net/openthread.h"

static void user_data_handler(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo) {
    UserData *data = (UserData *)aContext;
    otMessage *responseMessage = NULL;
    
    responseMessage = otCoapNewMessage(openthread_get_default_instance(), NULL);
    EXPECT_OR_EXIT(responseMessage != NULL);
    
    EXPECT_NO_ERROR_OR_EXIT(otCoapMessageInitResponse(responseMessage, aMessage, OT_COAP_TYPE_NON_CONFIRMABLE, OT_COAP_CODE_VALID));
    EXPECT_NO_ERROR_OR_EXIT(otCoapMessageSetPayloadMarker(responseMessage));
    EXPECT_NO_ERROR_OR_EXIT(otCoapMessageSetPayloadMarker(responseMessage));
    
    data->mUpdateHandler(data->mUserData);

    EXPECT_NO_ERROR_OR_EXIT(otMessageAppend(responseMessage, data->mUserData, data->len));
    EXPECT_NO_ERROR_OR_EXIT(otCoapSendResponse(openthread_get_default_instance(), responseMessage, aMessageInfo));

exit:
    if (responseMessage != NULL) {
        otMessageFree(responseMessage);
    }
}

void addCoAPResource(otCoapResource aResource) {

};