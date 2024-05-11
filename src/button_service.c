#include "button_service.h"
#include "action_button.h" 

#define     BT_BUTTON_NODE                           DT_ALIAS(bluetooth_button)
#define     DEVICE_BUTTON_NODE                       DT_ALIAS(device_button)
#define     BTN_WORKQUEUE_PRIORITY                   0
#define     BTN_QUEUE_STACK_SIZE                     512

struct action_button bt_button = {
    .dt_spec= GPIO_DT_SPEC_GET(BT_BUTTON_NODE, gpios),
    .press_status = released,
};

struct action_button device_button = {
    .dt_spec= GPIO_DT_SPEC_GET(DEVICE_BUTTON_NODE, gpios),
    .press_status = released,
};

static struct k_work_q button_work_q;

K_THREAD_STACK_DEFINE(btn_work_stack, BTN_QUEUE_STACK_SIZE);

static void init_work_q();

int init_button_service() {
    int err;
    init_work_q();
    err = init_button(&bt_button);
    err = init_button(&device_button);

    return err;
}

static void init_work_q() {
    k_work_queue_init(&button_work_q);
    k_work_queue_start(&button_work_q,
                       btn_work_stack, K_THREAD_STACK_SIZEOF(btn_work_stack),
                       BTN_WORKQUEUE_PRIORITY, NULL);
}