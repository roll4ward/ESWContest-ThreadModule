#include "button.h"
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#define     BTN_WORKQUEUE_PRIORITY                   0
#define     BTN_QUEUE_STACK_SIZE                     512
#define     BT_BUTTON_NODE                           DT_ALIAS(sw0)
#define     LONG_PRESS_BOUNDARY_MS                   1000LL

LOG_MODULE_REGISTER(button, LOG_LEVEL_INF);

struct action_button bt_button = {
    .dt_spec= GPIO_DT_SPEC_GET(BT_BUTTON_NODE, gpios),
    .press_status = released,
};

static struct k_work_q button_work_q;

K_THREAD_STACK_DEFINE(btn_work_stack, BTN_QUEUE_STACK_SIZE);

static void init_work_q();
static int init_button(struct action_button *button); // 버튼별로

static void button_isr(const struct device *dt, struct gpio_callback *cb, uint32_t pins);
static struct action_button *select_button_by_pin(int pin_num);
static void on_pressed(struct action_button* button);
static void on_released(struct action_button* button);
static void check_pressed_time(struct k_work *item);


int init_button_service() {
    int err;
    init_work_q();
    err = init_button(&bt_button);

    return err;
}
   

static void init_work_q() {
    k_work_queue_init(&button_work_q);
    k_work_queue_start(&button_work_q,
                       btn_work_stack, K_THREAD_STACK_SIZEOF(btn_work_stack),
                       BTN_WORKQUEUE_PRIORITY, NULL);
}

static int init_button(struct action_button *button) {
    int ret;

    k_event_init(&(button->press_event));
    k_work_init(&(button->work), check_pressed_time);

    if(!device_is_ready(button->dt_spec.port)) return -ENOTSUP;
    LOG_INF("GPIO device is ready");

    ret = gpio_pin_configure_dt(&(button->dt_spec), GPIO_INPUT);
    if(ret < 0) return ret;
    LOG_INF("Button is setted as INPUT");

    ret = gpio_pin_interrupt_configure_dt(&(button->dt_spec), GPIO_INT_EDGE_BOTH);
    if(ret < 0) return ret;
    LOG_INF("BOTH_EDGE interrupt is setted");

    gpio_init_callback(&(button->cb), button_isr, BIT(button->dt_spec.pin));
    LOG_INF("callback is initialized");
    
    ret = gpio_add_callback(button->dt_spec.port, &(button->cb));
    if(ret < 0) return ret;
    LOG_INF("callback is added");

    return 0;
}

void button_isr(const struct device *dt, struct gpio_callback *cb, gpio_port_pins_t pins) {
    LOG_DBG("Button interrupt service routine");
    int pin_num = (int)LOG2(cb->pin_mask);
    struct action_button *button = &bt_button; // TODO: pin_num으로 action_button 선택하는 로직

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

static struct action_button *select_button_by_pin(int pin_num) {
    return NULL;
}

static void on_pressed(struct action_button* button) {
    LOG_DBG("button just pressed: %d", button->press_status);
    k_work_submit(&(button->work));
}

static void on_released(struct action_button* button) {
    LOG_DBG("button just released: %d", button->press_status);
}

static void check_pressed_time(struct k_work *item) {
    uint32_t start_time = k_uptime_get_32();
    for (;;) {
        uint32_t time_delta = ( k_uptime_get_32() - start_time);
        if (time_delta > LONG_PRESS_BOUNDARY_MS) {  // Long Press
            k_event_set(&(bt_button.press_event), BT_BUTTON_LONG_PRESS);
            return;
        }

        else if (bt_button.press_status == released) {
            k_event_set(&(bt_button.press_event), BT_BUTTON_SHORT_PRESS);
            return;
        }
    }
}