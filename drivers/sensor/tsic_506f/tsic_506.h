/**
 * Copyright (c) 2024 Marko Vejnovic <mvejnovic@tesla.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_TSIC_506F_TSIC_506F_H_
#define ZEPHYR_DRIVERS_SENSOR_TSIC_506F_TSIC_506F_H_

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

#ifdef __cplusplus
extern "C" {
#endif

struct tsic_506f_config {
    struct gpio_dt_spec gpio;
};

struct tsic_506f_data {
    struct gpio_callback interrupt_cb_data;

    uint16_t value;
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_TSIC_506F_TSIC_506F_H_ */
