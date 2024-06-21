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
static void collect_data_10hz(struct experiment_row* experiment_row) {
    experiment_row_add_value(experiment_row, 42.0);
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
    do {
        LOG_INF("Waiting for trutime support.");
        k_msleep(1000);
    } while (!trutime_is_available(time_provider));
    k_msleep(1000); // TODO(markovejnovic): trutime_is_available leaks before
                    // it is actually available.

    // Initialize the experiment 
    struct experiment* experiment = experiment_init(storage, time_provider);

    // Populate the experiment with all the required columns.
    declare_columns(experiment);

    while (1) {
        const uint64_t start = k_uptime_get();

        // We are creating a new time sample -- create a new row.
        struct experiment_row* row = experiment_row_new(
            trutime_millis_since(
                time_provider,
                experiment_start_time(experiment)
            )
        );

        // Collect the specified data into the experiment.
        collect_data_10hz(row);

        // Push these values into the experiment.
        if ((err = experiment_push_row(experiment, row)) != 0) {
            LOG_ERR("Failed to push a row into the experiment (%d)", err);
        }

        const uint64_t stop = k_uptime_get();
        int64_t sleep_period;
        const bool sleep_will_underflow = __builtin_sub_overflow(
            (int64_t)stop,
            (int64_t)start,
            &sleep_period
        );
        if (sleep_will_underflow) {
            LOG_ERR("Critically slow application causing sampling lag.");
        } else {
            k_msleep(SAMPLING_PERIOD_MS - (sleep_period));
        }
    }
}
