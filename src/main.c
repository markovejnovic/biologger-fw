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
#include "zephyr/drivers/sensor.h"

LOG_MODULE_REGISTER(main);

OBSERVER_DECL(main_observer);
TRUTIME_DECL(time_provider);

#define TSIC DT_NODELABEL(tsic_506)
static const struct device* tsic_506 = DEVICE_DT_GET(TSIC);

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


        k_msleep(1000);
    }
}
