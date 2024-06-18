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
#include "experiment.h"
#include "observer.h"
#include <zephyr/device.h>
#include "trutime.h"
#include "storage.h"
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/adc.h>

#define SAMPLING_PERIOD_MS 100

LOG_MODULE_REGISTER(main);

OBSERVER_DECL(main_observer);
TRUTIME_DECL(time_provider);

#define TSIC DT_NODELABEL(tsic_506)
static const struct device* tsic_506 = DEVICE_DT_GET(TSIC);

#define ADS1115_DT DT_NODELABEL(ads1115_adc)
static const struct adc_dt_spec ads1115_adc = ADC_DT_SPEC_GET_BY_NAME(ADS1115_DT, a0);

/**
 * @brief Initialize drivers required for the operation of the application.
 */
static int init_drivers() {
    int err = 0;

    if (!adc_is_ready_dt(&ads1115_adc)) {
        LOG_ERR("The ADC does not appear to be ready.");
    }

    if ((err = adc_channel_setup_dt(&ads1115_adc)) != 0) {
        LOG_ERR("Failed to setup ADC channel. Error = %d", err);
    }
	uint16_t buf;
	struct adc_sequence sequence = {
		.buffer = &buf,
		/* buffer size in bytes, not number of samples */
		.buffer_size = sizeof(buf),
	};
    if ((err = adc_sequence_init_dt(&ads1115_adc, &sequence)) != 0) {
        LOG_ERR("Failed to init ADC sequence. Error = %d", err);
    }


    return err;
}

/**
 * @brief Define all the columns that are collected.
 *
 * This function must only contain calls to experiment_add_column.
 */
static void declare_columns(struct experiment* e) {
    experiment_add_column(e, "Clock Seconds", "s");
}

/**
 * @brief Perform all data collection.
 *
 * This function must only contain calls to experiment_row_add_value.
 *
 * @warning There must be as many calls to experiment_row_add_value as there
 *          are to experiment_add_column in declare_columns.
 */
static void collect_data_10hz(
    struct experiment_row* experiment_row,
    struct tm* time
) {
    experiment_row_add_value(experiment_row, time->tm_sec);
}

int main(void) {
    int err;

    // Initialize the logging module which is invaluable when debugging.
    LOG_INIT();
    LOG_INF("Initializing modules...");

    // Initialize the observer which will oversee all the operations and blink
    // the LED.
    observer_t observer = OBSERVER_INIT(main_observer);

    // Initialize the storage module which is responsible for storing
    // experiment data.
    storage_t storage = storage_init(observer);

    // Initialize the trutime module which provides with accurate time data.
    trutime_t time_provider = TRUTIME_INIT(time_provider, observer);

    // Initialize the hardware drivers.
    if ((err = init_drivers()) != 0) {
        LOG_ERR("Failed to initialize a driver (%d).", err);
        observer_flag_raise(observer, OBSERVER_FLAG_DRIVER_MIA);
    }

    // Wait until trutime is available. Sometimes this takes quite some time.
    do { k_msleep(1000); } while (!trutime_is_available(time_provider));

    // Initialize the experiment 
    struct experiment* experiment = experiment_init(storage);

    // Populate the experiment with all the required columns.
    declare_columns(experiment);

    while (1) {
        const uint32_t start = k_uptime_get_32();

        // Collect the current time.
        struct tm ts;
        if ((err = trutime_get_utc(time_provider, &ts)) != 0) {
            LOG_ERR("Could not retrieve the time.");
        } else {
            LOG_DBG("Time: %s\n", asctime(&ts));
        }

        // We are creating a new time sample -- create a new row.
        struct experiment_row* row = experiment_row_new();

        // Collect the specified data into the experiment.
        collect_data_10hz(row, &ts);

        // Push these values into the experiment.
        if ((err = experiment_push_row(experiment, row)) != 0) {
            LOG_ERR("Failed to push a row into the experiment (%d)", err);
        }

        const uint32_t stop = k_uptime_get_32();;
        k_msleep(SAMPLING_PERIOD_MS - (stop - start));
    }
}
