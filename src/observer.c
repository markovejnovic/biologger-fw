#include "observer.h"
#include "thread_specs.h"
#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(observer, CONFIG_LOG_DEFAULT_LEVEL);

#define STATUS_LED DT_NODELABEL(status_led)

static const struct gpio_dt_spec status_led =
    GPIO_DT_SPEC_GET_OR(STATUS_LED, gpios, {0});

static void observer_flag(observer_t o, enum observer_flag f, bool s) {
    o->flag_states[f] = s;

    o->app_state =
        o->flag_states[OBSERVER_FLAG_NO_DISK]
        || o->flag_states[OBSERVER_FLAG_DRIVER_MIA]
            ? APP_STATE_DEVICE_FAULT :
        o->flag_states[OBSERVER_FLAG_NO_GPS_CLOCK]
            ? APP_STATE_WAITING_FOR_TIMESYNC :
        APP_STATE_SAMPLING;
}

static void blink_status_runnable(void* p0, void* p1, void* p2) {
    observer_t observer = (observer_t)p0;

    // Repeatedly we will blink the LED as part of the LED thread.
    while (true) {
        uint16_t blink_period_ms = 1000;
        uint16_t high_side_time_ms = 100;

        switch (observer->app_state) {
            case APP_STATE_SAMPLING:
                blink_period_ms = 500;
                high_side_time_ms = 200;
                break;

            case APP_STATE_WAITING_FOR_TIMESYNC:
                blink_period_ms = 3000;
                high_side_time_ms = 500;
                break;

            case APP_STATE_DEVICE_FAULT:
                blink_period_ms = 1000;
                high_side_time_ms = 1000;
                break;
        }

        const uint16_t low_side_time_ms = blink_period_ms - high_side_time_ms;

        if (!gpio_pin_get_dt(&status_led)) {
            gpio_pin_set_dt(&status_led, 1);
            k_msleep(high_side_time_ms);
        } else {
            gpio_pin_set_dt(&status_led, 0);
            k_msleep(low_side_time_ms);
        }
    }
}

K_THREAD_STACK_DEFINE(work_thread_stack, THREAD_BLINK_STATUS0_STACK_SIZE);
static struct k_thread work_thread_data;

observer_t observer_start(observer_t observer) {
    observer->app_state = APP_STATE_DEVICE_FAULT;

    for (int i = 0; i < OBSERVER_FLAG_COUNT; i++) {
        observer_flag(observer, i, false);
    }

    if (!gpio_is_ready_dt(&status_led)) {
        LOG_ERR("Status LED @ %s:%d is not ready. Failing to initialize.",
                status_led.port->name, status_led.pin);
        return NULL;
    }

    if (gpio_pin_configure_dt(&status_led, GPIO_OUTPUT_ACTIVE) < 0) {
        LOG_ERR("Status LED @ %s:%d failed to configure GPIO active.",
                status_led.port->name, status_led.pin);
        return NULL;
    }

    k_thread_create(
        &work_thread_data,
        work_thread_stack,
        K_THREAD_STACK_SIZEOF(work_thread_stack),
        blink_status_runnable, observer, NULL, NULL,
        THREAD_BLINK_STATUS0_PRIORITY, 0, K_NO_WAIT
    );

    return observer;
}

void observer_flag_raise(observer_t o, enum observer_flag f) {
    observer_flag(o, f, true);
}

void observer_flag_lower(observer_t o, enum observer_flag f) {
    observer_flag(o, f, false);
}
