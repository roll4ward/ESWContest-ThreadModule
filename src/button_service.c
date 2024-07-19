#include "button_service.h"
#include "action_button.h" 

static bool button_service_ready = false;

static sys_slist_t button_list;

void init_button_service() {
    sys_slist_init(&button_list);

    button_service_ready = true;
}

int is_button_service_ready() {
    return button_service_ready;
}

int register_button(struct action_button *button, struct action_button_callback *callback) {
    int err;

    if (!button_service_ready) init_button_service();
    err = init_button(button, callback);

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