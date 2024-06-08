#include "observer.h"
#include "thread_specs.h"
#include <stdatomic.h>
#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(observer, CONFIG_LOG_DEFAULT_LEVEL);

#define STATUS_LED DT_NODELABEL(status_led)
#if !DT_NODE_HAS_STATUS(STATUS_LED, okay)
#error "Unsupported board: led0 devicetree alias is not defined"
#endif

static const struct gpio_dt_spec status_led =
    GPIO_DT_SPEC_GET_OR(STATUS_LED, gpios, {0});

void blink_status_runnable(void* p0, void* p1, void* p2) {
    observer_t observer = (observer_t)p0;

    while (true) {
        observer_state current_state = atomic_load_explicit(
            &observer->current_state, memory_order_relaxed
        );

        uint16_t blink_period_ms = 1000;
        uint16_t high_side_time_ms = 100;
        switch (current_state) {
            case OBSERVER_STATE_FAIL_INIT:
                blink_period_ms = 200;
                high_side_time_ms = 100;
                break;
            case OBSERVER_STATE_INITIALIZING:
                blink_period_ms = 100;
                high_side_time_ms = 50;
                break;
            case OBSERVER_STATE_WAITING_FOR_DISK:
                blink_period_ms = 1000;
                high_side_time_ms = 0;
                break;
            case OBSERVER_STATE_OPERATING_NORMALLY:
                blink_period_ms = 1000;
                high_side_time_ms = 300;
                break;
        }

        const uint16_t low_side_time_ms = blink_period_ms- high_side_time_ms;


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
    atomic_store_explicit(
        &observer->current_state,
        OBSERVER_STATE_OPERATING_NORMALLY,
        memory_order_relaxed
    );

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

void observer_set_state(observer_t observer, observer_state state) {
    atomic_store_explicit(
        &observer->current_state,
        state,
        memory_order_relaxed
    );
}

void observer_flag(observer_t o, enum observer_flag f, bool s) {
    switch (f) {
        case OBESRVER_FLAG_NO_DISK:
            // TODO(markovejnovic): Rearchitect the observer.
            observer_set_state(o, s
                ? OBSERVER_STATE_WAITING_FOR_DISK
                : OBSERVER_STATE_OPERATING_NORMALLY);
            break;
    }
}
