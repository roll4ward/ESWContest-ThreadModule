#include "button.h"
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#define BTN_WORKQUEUE_PRIORITY 5
#define BTN_QUEUE_STACK_SIZE 512
K_THREAD_STACK_DEFINE(btn_work_stack, BTN_QUEUE_STACK_SIZE);

#define BT_BUTTON_NODE DT_ALIAS(bluetooth_button)

static struct k_work_q btn_work_q;

static void init_work_q() {
    k_work_queue_init(&btn_work_q);

    k_work_queue_start(&btn_work_q,
                       btn_work_stack, K_THREAD_STACK_SIZEOF(btn_work_stack),
                       BTN_WORKQUEUE_PRIORITY, NULL);
}


static struct gpio_dt_spec bt_button = GPIO_DT_SPEC_GET(BT_BUTTON_NODE, gpios);

int init_button_service() {
    if(!device_is_ready(bt_button.port)) return -1;
    
    gpio_pin_configure_dt(&bt_button, GPIO_INPUT);
}