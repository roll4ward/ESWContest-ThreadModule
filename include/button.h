#ifndef ROLL4_BUTTON_H
#define ROLL_4_BUTTON_H

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#define     BT_BUTTON_SHORT_PRESS                    BIT(1)
#define     BT_BUTTON_LONG_PRESS                     BIT(2)

enum button_status {
    released = 0,
    pressed,
};

struct action_button{
    struct k_work work;
    struct gpio_callback cb;
    struct gpio_dt_spec dt_spec;
    enum button_status press_status;
    struct k_event press_event;
};

int init_button_service();
#endif