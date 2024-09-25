#include "action_button.h"
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "button_service.h"

LOG_MODULE_REGISTER(button, LOG_LEVEL_INF);

static void button_isr(const struct device *dt, struct gpio_callback *cb, gpio_port_pins_t pins);
static void on_pressed(struct action_button* button);
static void on_released(struct action_button* button);
static void check_pressed_time(struct k_work *item);

int init_button(struct action_button *button, struct action_button_callback *callback) {
    int ret;
    if (callback->on_long_press) k_work_init(&(button->long_press_work), callback->on_long_press);
    if (callback->on_short_press) k_work_init(&(button->short_press_work), callback->on_short_press);
    k_work_init(&(button->press_time_check_work), check_pressed_time);

    if(!device_is_ready(button->dt_spec.port)) return -ENOTSUP;
    LOG_INF("GPIO device is ready");

    ret = gpio_pin_configure_dt(&(button->dt_spec), GPIO_INPUT);
    if(ret < 0) return ret;
    LOG_INF("Button is setted as INPUT");

    ret = gpio_pin_interrupt_configure_dt(&(button->dt_spec), GPIO_INT_EDGE_BOTH);
    if(ret < 0) return ret;
    LOG_INF("BOTH_EDGE interrupt is setted");

    gpio_init_callback(&(button->gpio_callback), button_isr, BIT(button->dt_spec.pin));
    LOG_INF("callback is initialized");
    
    ret = gpio_add_callback(button->dt_spec.port, &(button->gpio_callback));
    if(ret < 0) return ret;
    LOG_INF("callback is added");

    return 0;
}

void button_isr(const struct device *dt, struct gpio_callback *cb, gpio_port_pins_t pins) {
    LOG_DBG("Button interrupt service routine");
    int pin_num = (int)LOG2(cb->pin_mask);
    struct action_button *button;
    if (!(button = select_button_by_pin(pin_num))) return;

    LOG_DBG("Select Pin Successful %d", button->dt_spec.pin);
    button->press_status = gpio_pin_get(dt, pin_num);
    switch (button->press_status){
    case pressed:
        on_pressed(button);
        break;
    case released:
        on_released(button);
        break;
    default:
        break;
    }
}

static void on_pressed(struct action_button* button) {
    LOG_DBG("button just pressed: %d", button->press_status);
    k_work_submit(&(button->press_time_check_work));
}

static void on_released(struct action_button* button) {
    LOG_DBG("button just released: %d", button->press_status);
}

static void check_pressed_time(struct k_work *item) {
    int64_t start_time = k_uptime_get();
    struct action_button *button;
    button = CONTAINER_OF(item, struct action_button, press_time_check_work);
    LOG_DBG("Check pressed time");
    for (;;) {
        int64_t time_delta = ( k_uptime_get() - start_time);
        if (time_delta > button->threshold) {  // Long Press
            // Handler가 없을 경우 제대로된 처리 중
            if (button->long_press_work.handler) k_work_submit(&(button->long_press_work));
            return;
        }

        else if (button->press_status == released) {
            if (button->short_press_work.handler) k_work_submit(&(button->short_press_work));
            return;
        }
    }
}