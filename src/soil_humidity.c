#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>

#include "soil_humidity.h"
#include "coap.h"
#include <math.h>

LOG_MODULE_REGISTER(soil_humidity, LOG_LEVEL_INF);

static const struct adc_dt_spec adc_channel = ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user),0);

static int16_t buf;
static struct adc_sequence sequence = {
    .buffer = &buf,
    .buffer_size = sizeof(buf),
};

static void update_sensor(UserData *aUserData, double value) {
    (*(double *)(aUserData->mUserData)) = get_soil_humidity_value();
}

DEFINE_COAP_USER_DATA(double, soil_humidity_value, update_sensor);
DEFINE_COAP_RESOURCE(soil_humidity, &soil_humidity_value);

otCoapResource *get_soil_humidity_resource() {
    return &COAP_RESOURCE(soil_humidity);
}

int init_soil_humidity(){
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

double get_soil_humidity_value(){
    int err;

    err = adc_read(adc_channel.dev, &sequence);
	if (err < 0) {
		LOG_ERR("Could not read (%d)", err);
		return -1.0;
	}

    return round((double) (1.0 - (int)buf / 4096.0 ) * 100.0 * 100.0) / 100.0;
}