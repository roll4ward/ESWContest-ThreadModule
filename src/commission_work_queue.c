#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

#include "ble_commissioning.h"

#define COMMISSION_QUEUE_STACK_SIZE                              4096
#define COMMISSION_WORKQUEUE_PRIORITY                            3

LOG_MODULE_REGISTER(ble_commission, LOG_LEVEL_INF);

static struct k_work_q commission_work_q;
K_THREAD_STACK_DEFINE(commission_work_stack, COMMISSION_QUEUE_STACK_SIZE);

void init_ble_commissioning_service() {
    k_work_queue_init(&commission_work_q);
    k_work_queue_start(&commission_work_q, commission_work_stack,
                        K_THREAD_STACK_SIZEOF(commission_work_stack),
                        COMMISSION_WORKQUEUE_PRIORITY, NULL);
}

struct k_work_q *get_commission_work_q() {
    return &commission_work_q;
}