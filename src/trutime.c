#include "memex.h"
#include "observer.h"
#include "trutime.h"
#include <stdatomic.h>
#include <stdbool.h>
#include <string.h>
#include <sys/errno.h>
#include <time.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gnss.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/logging/log.h>

#define CONFIG_TRUTIME_MOCK_GNSS
#ifdef CONFIG_TRUTIME_MOCK_GNSS
#warning You are currently using trutime without actual GNSS support. This \
will result in incorrect time information.
#endif

#define DT_GNSS DT_NODELABEL(gnss)
#define DT_GNSS_DISABLE DT_NODELABEL(gnss_disable)
#define DT_RTC DT_ALIAS(trutime_clock)
#define CURRENT_CENTURY_YEAR 2000

LOG_MODULE_REGISTER(trutime);

static const struct device * const rtc_dev = DEVICE_DT_GET(DT_RTC);
static const struct device * const gnss_dev = DEVICE_DT_GET(DT_GNSS);
static const struct gpio_dt_spec gnss_disable =
    GPIO_DT_SPEC_GET(DT_GNSS_DISABLE, gpios);
static observer_t observer = NULL;
static _Atomic bool synced_rtc_with_gps = false;

static struct tm gnss_time_to_tm(const struct gnss_time* t) {
    const int zero_indexed_mo = t->month - 1u;
    const int year_relative_to_1900 =
        t->century_year + CURRENT_CENTURY_YEAR - 1900;

    struct tm time = {
        .tm_sec = t->millisecond / 1000,
        .tm_min = t->minute,
        .tm_hour = t->hour,
        .tm_mday = t->month_day,
        .tm_mon = zero_indexed_mo,
        .tm_year = year_relative_to_1900,
        .tm_wday = -1,
        .tm_yday = -1,
        .tm_isdst = -1,
    };
    mktime(&time);
    return time;
}

static void gnss_data_callback(
    const struct device* dev,
    const struct gnss_data* data
) {
    if (observer == NULL) {
        // App is not initialiezd, wait!
        return;
    }

    if (data->info.fix_status != GNSS_FIX_STATUS_GNSS_FIX) {
        // Don't do anything unless we have a fix -- we won't have data
        // anyways.
        return;
    }


    if (CMP_ALL((uint8_t*)&data->utc, sizeof(data->utc), 0)) {
        // If all the timestamp values are 0, then there is no point in trying
        // anything. We have no data.
        return;
    }

    int errno;
    LOG_INF("Received GNSS timestamp: %d-%d-%d %d:%d:%d.%d.",
            data->utc.century_year, data->utc.month, data->utc.month_day,
            data->utc.hour, data->utc.minute, data->utc.millisecond / 1000,
            data->utc.millisecond % 1000);

    const struct tm time = gnss_time_to_tm(&data->utc);

    if ((errno = rtc_set_time(rtc_dev, (const struct rtc_time*)&time)) != 0) {
        LOG_ERR("Failed to update the RTC time. Error: %d", errno);
        return;
    }

    if ((errno = gpio_pin_set_dt(&gnss_disable, 1)) != 0) {
        LOG_ERR("Failed to disable the GNSS module. Error: %d", errno);
    }

    LOG_INF("Aligned the RTC to GNSS.");
    observer_flag_lower(observer, OBSERVER_FLAG_NO_GPS_CLOCK);
    atomic_store_explicit(&synced_rtc_with_gps, true, memory_order_release);
}

GNSS_DATA_CALLBACK_DEFINE(gnss_dev, gnss_data_callback);

trutime_t trutime_init(trutime_t trutime, observer_t obs) {
    int errno;

    observer = obs;
    observer_flag_raise(observer, OBSERVER_FLAG_NO_GPS_CLOCK);

    if (!device_is_ready(rtc_dev)) {
        LOG_ERR("Cannot initialize trutime because the RTC device is not "
                "ready.");
        return NULL;
    }

    if (!device_is_ready(gnss_dev)) {
        LOG_ERR("Cannot initialize trutime because the GNSS device is not "
                "ready.");
        return NULL;
    }

    if (!gpio_is_ready_dt(&gnss_disable)) {
        LOG_ERR("GNSS Disable @ %s:%d is not ready. Failing to initialize.",
                gnss_disable.port->name, gnss_disable.pin);
        return NULL;
    }

    if (gpio_pin_configure_dt(&gnss_disable, GPIO_OUTPUT_ACTIVE) < 0) {
        LOG_ERR("GNSS Disable @ %s:%d failed to configure GPIO active.",
                gnss_disable.port->name, gnss_disable.pin);
        return NULL;
    }

    if ((errno = gpio_pin_set_dt(&gnss_disable, 0)) != 0) {
        LOG_ERR("Couldn't enable the GNSS module.");
    }

    // If we are mocking the time due to the absense of a GNSS chip let's set
    // some fake value.
#ifdef CONFIG_TRUTIME_MOCK_GNSS
    struct tm fake_time;
    fake_time.tm_year = 2023 - 1900;
    fake_time.tm_mon = 8;
    fake_time.tm_mday = 9;
    mktime(&fake_time);
    rtc_set_time(rtc_dev, (struct rtc_time*)&fake_time);
    atomic_store_explicit(&synced_rtc_with_gps, true, memory_order_release);
    observer_flag_lower(observer, OBSERVER_FLAG_NO_GPS_CLOCK);
#endif

    LOG_INF("Initialized trutime.");
    return trutime;
}

int trutime_get_utc(trutime_t trutime, struct rtc_time* out) {
    int errno;

    if (!atomic_load_explicit(&synced_rtc_with_gps, memory_order_acquire)) {
        return -ENODATA;
    }

    if ((errno = rtc_get_time(rtc_dev, out)) != 0) {
        LOG_ERR("Failed to perform rtc_get_time. Error = %d", errno);
        return errno;
    }

    return 0;
}

long long trutime_millis_since(trutime_t t, const struct rtc_time *since) {
    struct rtc_time t_now;
    int err;

    if ((err = trutime_get_utc(t, &t_now)) != 0) {
        LOG_ERR("Could not retrieve the current time in an attempt to compute "
                "delta (%d).", err);
        return err;
    }

    const double diff_secs = difftime(mktime((struct tm*)&t_now),
                                      mktime((struct tm*)since));
    if (diff_secs < 0) {
        LOG_ERR("Computed negative trutime_millis_since.");
        return -EINVAL;
    }

    return (long long)(diff_secs * 1000)
        + ((t_now.tm_nsec - since->tm_nsec) / 1000000);
}

bool trutime_is_available(trutime_t t) {
    (void)t;

    return atomic_load_explicit(&synced_rtc_with_gps, memory_order_acquire);
}
