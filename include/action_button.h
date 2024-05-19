#ifndef ROLL4_ACTION_BUTTON_H
#define ROLL4_ACTION_BUTTON_H

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#define     BT_BUTTON_SHORT_PRESS                    BIT(1)
#define     BT_BUTTON_LONG_PRESS                     BIT(2)

enum button_status {
    released = 0,
    pressed,
};

struct action_button{
    struct k_work press_time_check_work;
    struct gpio_dt_spec dt_spec;
    enum button_status press_status;
    struct k_event press_event;
    sys_snode_t registry_node;
};

int init_button(struct action_button *button);
#endif