#include <zephyr/kernel.h>
#include <zephyr/net/openthread.h>

#include "thread_net_config.h"
int main(void)
{
        struct openthread_context *otContext = openthread_get_default_context();

        openthread_start(otContext);
        return 0;
}