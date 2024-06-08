/*
 * Copyright (c) 2024 Marko Vejnovic
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <sys/_timespec.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/shell/shell.h>
#include "observer.h"
#include <zephyr/device.h>
#include "trutime.h"
#include "storage.h"

LOG_MODULE_REGISTER(main);

OBSERVER_DECL(main_observer);
TRUTIME_DECL(time_provider);

int main(void) {
    int errno;

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

    trutime_t time_provider = TRUTIME_INIT(time_provider);

    while (1) {
        struct tm ts;
        if ((errno = trutime_get_utc(time_provider, &ts)) != 0) {
            LOG_ERR("Could not retrieve the time.\n");
        } else {
            printk("Time: %d-%d-%d %d:%d:%d\n",
                    ts.tm_year, ts.tm_mon, ts.tm_mday,
                    ts.tm_hour, ts.tm_min, ts.tm_sec);
        }
        k_msleep(1000);
    }
}
