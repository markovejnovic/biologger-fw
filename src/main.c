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
#include "zephyr/drivers/gpio.h"

LOG_MODULE_REGISTER(main);

OBSERVER_DECL(main_observer);
TRUTIME_DECL(time_provider);

#define TEMPSENS DT_NODELABEL(zac_wire)
static const struct gpio_dt_spec tempsens = GPIO_DT_SPEC_GET(TEMPSENS, gpios);

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

    if (!gpio_is_ready_dt(&tempsens)) {
        LOG_ERR("Cannot initialize the temperature sensor. GPIO %s:%d is "
                "not ready", tempsens.port->name, tempsens.pin);
    }
    if ((errnum = gpio_pin_configure_dt(&tempsens, GPIO_INPUT)) != 0) {
        LOG_ERR("Failed to configure %s:%d as input due to error %d",
                tempsens.port->name, tempsens.pin, errnum);
    }

    while (1) {
        struct tm ts;
        if ((errnum = trutime_get_utc(time_provider, &ts)) != 0) {
            LOG_ERR("Could not retrieve the time.\n");
        } else {
            printk("Time: %s", asctime(&ts));
        }
        k_msleep(1000);
    }
}
