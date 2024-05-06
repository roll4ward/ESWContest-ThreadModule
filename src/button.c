#include "button.h"
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#define     BTN_WORKQUEUE_PRIORITY                   5
#define     BTN_QUEUE_STACK_SIZE                     512
#define     BT_BUTTON_NODE                           DT_ALIAS(bluetooth_button)

static enum button_status {
    released = 0,
    pressed,
};

static struct k_work_q btn_work_q;
static struct gpio_callback bt_button_cb;
static struct gpio_dt_spec bt_button = GPIO_DT_SPEC_GET(BT_BUTTON_NODE, gpios);
static enum button_status bt_button_status = released;

K_THREAD_STACK_DEFINE(btn_work_stack, BTN_QUEUE_STACK_SIZE);

static void init_work_q();
static int init_bt_button();

static void bt_button_isr(const struct device *dt, struct gpio_callback *cb, uint32_t pins);
static void on_pressed();
static void on_released();


int init_button_service() {
    init_work_q();
    init_bt_button();
}
   

static void init_work_q() {
    k_work_queue_init(&btn_work_q);

    k_work_queue_start(&btn_work_q,
                       btn_work_stack, K_THREAD_STACK_SIZEOF(btn_work_stack),
                       BTN_WORKQUEUE_PRIORITY, NULL);
}

static int init_bt_button() {
    int ret;

    if(!device_is_ready(bt_button.port)) return -ENOTSUP;

    ret = gpio_pin_configure_dt(&bt_button, GPIO_INPUT);
    if(!ret) return ret;

    ret = gpio_pin_interrupt_configure_dt(&bt_button, GPIO_INT_EDGE_BOTH);
    if(!ret) return ret;

    gpio_init_callback(&bt_button_cb, bt_button_isr, BIT(bt_button.pin));
    
    ret = gpio_add_callback_dt(&bt_button, &bt_button_cb);
    if(!ret) return ret;

    return 0;
}


void bt_button_isr(const struct device *dt, struct gpio_callback *cb, uint32_t pins) {
    switch (bt_button_status){
    case pressed:
        bt_button_status = released;
        on_released();
        break;
    case released:
        bt_button_status = pressed;
        on_pressed();
        break;
    default:
        break;
    }
}

static void on_pressed() {

}

static void on_released() {

}