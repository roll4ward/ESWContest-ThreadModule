#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/device.h>

#include "temperature_humidity.h"
#include "coap.h"
#include <math.h>

LOG_MODULE_REGISTER(tempertature_humidity, LOG_LEVEL_INF);

static struct device *dht_dev = DEVICE_DT_GET(DT_NODELABEL(dht11));
static struct sensor_value temperature, humidity;

static int64_t last_fetch_time = 0;

static int fetch_sensor() {
    if (k_uptime_delta(&last_fetch_time) < 50) {
        LOG_INF("PASS fetch");
        return 0;
    }

    int ret;
    if (!device_is_ready(dht_dev)) {
		LOG_ERR("sensor: device not ready.\n");
		return -1;
	}

    ret = sensor_sample_fetch(dht_dev);
    if(ret < 0) {
        LOG_ERR("sensor: fetch failed.\n");
		return -1;
    }

    last_fetch_time = k_uptime_get();
    return 0;
}

double get_temperature_value() {
    int ret;

    fetch_sensor();

    ret = sensor_channel_get(dht_dev, SENSOR_CHAN_AMBIENT_TEMP, &temperature);
    if(ret < 0) {
        LOG_ERR("temperature: get failed.\n");
		return -1.0;
    }

    return round(sensor_value_to_double(&temperature) * 100.0) / 100.0;
}

double get_humidity_value() {
    int ret;

    fetch_sensor();
    
    ret = sensor_channel_get(dht_dev, SENSOR_CHAN_HUMIDITY, &humidity);
    if(ret < 0) {
        LOG_ERR("humidity: get failed.\n");
		return -1.0;
    }

    return round(sensor_value_to_double(&humidity) * 100.0) / 100.0;
}

static void update_temperature(UserData *aUserData, double value) {
    (*(double *)(aUserData->mUserData)) = get_temperature_value();
}

static void update_humidity(UserData *aUserData, double value) {
    (*(double *)(aUserData->mUserData)) = get_humidity_value();
}


DEFINE_COAP_USER_DATA(double, temperature_value, update_temperature);
DEFINE_COAP_RESOURCE(temperature, &temperature_value);

otCoapResource *get_temperature_resource() {
    return &COAP_RESOURCE(temperature);
}

DEFINE_COAP_USER_DATA(double, humidity_value, update_humidity);
DEFINE_COAP_RESOURCE(humidity, &humidity_value);

otCoapResource *get_humidity_resource() {
    return &COAP_RESOURCE(humidity);
}