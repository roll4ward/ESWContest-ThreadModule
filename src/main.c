#include <zephyr/kernel.h>
#include <zephyr/net/openthread.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>

#include <openthread/thread.h>
#include <openthread/ip6.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#include "bluetooth_ad.h"
#include "ble_commissioning.h"
#include "coap.h"
#include "util.h"
#include "action_button.h"
#include "button_service.h"
#include "reset_network.h"
#include "cds.h"
#include "soil_humidity.h"
#include "temperature_humidity.h"

int main(void)
{
        init_ble_commission();
        init_button_service();
        init_reset_network();

        if(otDatasetIsCommissioned(openthread_get_default_instance())) {
                openthread_api_mutex_lock(openthread_get_default_context());
                otIp6SetEnabled(openthread_get_default_instance(), true);
                otThreadSetEnabled(openthread_get_default_instance(), true);
                openthread_api_mutex_unlock(openthread_get_default_context());
                LOG_INF("Start Thread");
        }

        else {
                start_bt_advertise();
                LOG_INF("Start BLE");
        }

        LOG_INF("START: CoAP Start");
        EXPECT_NO_ERROR_OR_DO(otCoapStart(openthread_get_default_instance(), 6000), LOG_ERR("Failed to Start CoAP"));
        LOG_INF("END: CoAP Start");

        EXPECT_NO_ERROR_OR_DO(init_cds(), LOG_ERR("Failed to start cds"));
        LOG_INF("END: CDS Start");
        EXPECT_NO_ERROR_OR_DO(init_soil_humidity(), LOG_ERR("Failed to start soil_humidity"));
        LOG_INF("END: SoilHumidity Start");

        #ifdef CONFIG_ROLL4_SENSOR
        addCoAPResource(get_soil_humidity_resource());
        addCoAPResource(get_cds_resource());
        addCoAPResource(get_temperature_resource());
        addCoAPResource(get_humidity_resource());
        #endif
        
        return 0;
}