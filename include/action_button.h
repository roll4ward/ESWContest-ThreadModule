#ifndef ROLL4_ACTION_BUTTON_H
#define ROLL4_ACTION_BUTTON_H

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

typedef k_work_handler_t button_callback_t;

enum button_status {
    released = 0,
    pressed,
};

struct action_button_callback {
    button_callback_t on_long_press;
    button_callback_t on_short_press;
};

struct action_button{
    struct k_work press_time_check_work;
    struct gpio_dt_spec dt_spec;
    struct gpio_callback gpio_callback;
    enum button_status press_status;
    struct k_work long_press_work;
    struct k_work short_press_work;
    sys_snode_t registry_node;
};

int init_button(struct action_button *button, struct action_button_callback *callback);
#endif