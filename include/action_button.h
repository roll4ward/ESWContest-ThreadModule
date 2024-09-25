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
    int64_t threshold;
    struct k_work press_time_check_work;
    struct gpio_dt_spec dt_spec;
    struct gpio_callback gpio_callback;
    enum button_status press_status;
    struct k_work long_press_work;
    struct k_work short_press_work;
    sys_snode_t registry_node;
};

#define DEFINE_ACTION_BUTTON(name, node, time) struct action_button name = { \
    .threshold = time,\
    .dt_spec= GPIO_DT_SPEC_GET(node, gpios),\
    .press_status = released,\
    .long_press_work = {0},\
    .short_press_work = {0}\
}

#define BUTTON_CALLBACK(name) void name(struct k_work *work)

int init_button(struct action_button *button, struct action_button_callback *callback);
#endif