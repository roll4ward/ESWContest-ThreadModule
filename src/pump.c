#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h> 
#include <zephyr/logging/log.h>

#include "pump.h"

LOG_MODULE_REGISTER(pump, LOG_LEVEL_INF);

#define PWM_PERIOD_NS                   10000000
#define PUMP                            DT_PATH(zephyr_user)
#define LED                             DT_ALIAS(red_pwm_led)

static const struct pwm_dt_spec pump = PWM_DT_SPEC_GET(LED);

int set_pump_value(double value) {
    int err;

    if(!pwm_is_ready_dt(&pump)) {
        LOG_ERR("Error: PWM device %s is not ready\n", pump.dev->name);
	    return -1;
    }

    err = pwm_set_dt(&pump, PWM_PERIOD_NS, (uint32_t)(value / 100 * PWM_PERIOD_NS));
    if (err < 0) {
        LOG_ERR("Error in pwm_set_dt(), err: %d", err);
        return -1;
    }
}