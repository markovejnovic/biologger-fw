/*
 * Copyright (c) 2024 Marko Vejnovic
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <sys/_timespec.h>
#include <time.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/shell/shell.h>
#include "observer.h"
#include <zephyr/device.h>
#include "trutime.h"
#include "storage.h"
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/adc.h>

LOG_MODULE_REGISTER(main);

OBSERVER_DECL(main_observer);
TRUTIME_DECL(time_provider);

#define TSIC DT_NODELABEL(tsic_506)
static const struct device* tsic_506 = DEVICE_DT_GET(TSIC);

#define ADS1115_DT DT_NODELABEL(ads1115_adc)
static const struct adc_dt_spec ads1115_adc = ADC_DT_SPEC_GET_BY_NAME(ADS1115_DT, a0);

int main(void) {
    int errnum;

    LOG_INIT();
    LOG_MODULE_DECLARE(main);
    LOG_INF("Initializing modules...");

    // Initialize the observer which will oversee all the operations and notify
    // the user.
    observer_t observer = OBSERVER_INIT(main_observer);
    observer_set_state(observer, OBSERVER_STATE_INITIALIZING);

    // Initialize the storage module which is responsible for storing
    // experiment data.
    storage_t storage = storage_init(observer);

    trutime_t time_provider = TRUTIME_INIT(time_provider, observer);

    if (!device_is_ready(tsic_506)) {
        LOG_ERR("The TSIC506 does not appear to be ready.");
    }

    if (!adc_is_ready_dt(&ads1115_adc)) {
        LOG_ERR("The ADC does not appear to be ready.");
    }

    if ((errnum = adc_channel_setup_dt(&ads1115_adc)) != 0) {
        LOG_ERR("Failed to setup ADC channel. Error = %d", errnum);
    }

	uint16_t buf;
	struct adc_sequence sequence = {
		.buffer = &buf,
		/* buffer size in bytes, not number of samples */
		.buffer_size = sizeof(buf),
	};

    if ((errnum = adc_sequence_init_dt(&ads1115_adc, &sequence)) != 0) {
        LOG_ERR("Failed to init ADC sequence. Error = %d", errnum);
    }

    while (1) {
        struct tm ts;
        if ((errnum = trutime_get_utc(time_provider, &ts)) != 0) {
            LOG_ERR("Could not retrieve the time.");
        } else {
            printk("Time: %s\n", asctime(&ts));
        }

        if ((errnum = sensor_sample_fetch(tsic_506)) != 0) {
            LOG_ERR("Failed to fetch the ambient temp: %d", errnum);
        }

        struct sensor_value sv;
        if ((errnum = sensor_channel_get(tsic_506, SENSOR_CHAN_AMBIENT_TEMP, &sv)) != 0) {
            LOG_ERR("Failed to measure the ambient temp: %d", errnum);
        } else {
            const double svf = sensor_value_to_double(&sv);
            printk("Temperature: %f\n", svf);
        }

        if ((errnum = adc_read_dt(&ads1115_adc, &sequence)) != 0) {
            LOG_ERR("Could not read sequence. Error = %d", errnum);
        }

        int32_t val_mv = (int32_t)buf;
        if ((errnum = adc_raw_to_millivolts_dt(&ads1115_adc, &val_mv)) != 0) {
            LOG_ERR("Failed to convert ADC raw to mV. Error = %d", errnum);
        }
        printk("ADC = %"PRId32" mV\n", val_mv);

        k_msleep(1000);
    }
}
