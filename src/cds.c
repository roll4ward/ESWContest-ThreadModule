#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>

#include "cds.h"
#include "coap.h"
#include <math.h>

LOG_MODULE_REGISTER(cds, LOG_LEVEL_INF);

static const struct adc_dt_spec adc_channel = ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user),1);

static int16_t buf;
static struct adc_sequence sequence = {
    .buffer = &buf,
    .buffer_size = sizeof(buf),
};

static void update_sensor(UserData *aUserData, double value) {
    (*(double *)(aUserData->mUserData)) = get_cds_value();
}

DEFINE_COAP_USER_DATA(double, cds_value, update_sensor);
DEFINE_COAP_RESOURCE(light, &cds_value);

otCoapResource *get_cds_resource() {
    return &COAP_RESOURCE(light);
}


int init_cds(){
    int err;
    if (!adc_is_ready_dt(&adc_channel)) {
        LOG_ERR("ADC controller devivce %s not ready", adc_channel.dev->name);
	    return -1;
    }

    err = adc_channel_setup_dt(&adc_channel);
    if (err < 0) {
	    LOG_ERR("Could not setup channel #%d (%d)", 0, err);
	    return err;
    }

    err = adc_sequence_init_dt(&adc_channel, &sequence);
	if (err < 0) {
		LOG_ERR("Could not initalize sequnce");
		return err;
	}

    return err;
}

double get_cds_value() {
    int err;

    err = adc_read(adc_channel.dev, &sequence);
	if (err < 0) {
		LOG_ERR("Could not read (%d)", err);
		return -1.0;
	}

    return round((double) (1.0 - (int)buf / 4096.0 ) * 100.0 * 100.0) / 100.0;
}