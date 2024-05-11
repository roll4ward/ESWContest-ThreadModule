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

static bool button_service_ready = false;
static struct k_work_q button_work_q;
sys_slist_t button_list;

K_THREAD_STACK_DEFINE(btn_work_stack, BTN_QUEUE_STACK_SIZE);

static void init_work_q();

void init_button_service() {
    init_work_q();
    sys_slist_init(&button_list);

    button_service_ready = true;
}

static void init_work_q() {
    k_work_queue_init(&button_work_q);
    k_work_queue_start(&button_work_q,
                       btn_work_stack, K_THREAD_STACK_SIZEOF(btn_work_stack),
                       BTN_WORKQUEUE_PRIORITY, NULL);
}

int is_button_service_ready() {
    return button_service_ready;
}

int register_button(struct action_button *button) {
    int err;

    if (!button_service_ready) init_button_service();
    err = init_button(button);

    if (err < 0) return err;

    sys_slist_append(&button_list, &(button->registry_node));

    return 0;
}

struct action_button *select_button_by_pin(int pin_num) {
    struct action_button *button = NULL;
    SYS_SLIST_FOR_EACH_CONTAINER(&button_list, button, registry_node) {
        if ((button->dt_spec.pin) == pin_num) return button;
    }

    return NULL;
}