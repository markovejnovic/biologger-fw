#include "trutime.h"
#include <string.h>
#include <sys/errno.h>
#include <time.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gnss.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>

#define DT_GNSS DT_NODELABEL(gnss)
#define DT_GNSS_DISABLE DT_NODELABEL(gnss_disable)
#define DT_RTC DT_ALIAS(trutime_clock)

LOG_MODULE_REGISTER(trutime);

static const struct device * const rtc_dev = DEVICE_DT_GET(DT_RTC);
static const struct device * const gnss_dev = DEVICE_DT_GET(DT_GNSS);
static const struct gpio_dt_spec gnss_disable =
    GPIO_DT_SPEC_GET(DT_GNSS_DISABLE, gpios);

static bool synced_rtc_with_gps = false;

void gnss_data_callback(
    const struct device* dev,
    const struct gnss_data* data
) {
    int errno;
    LOG_DBG("Received GNSS timestamp: %d-%d-%d %d:%d:%d.%d.",
            data->utc.century_year, data->utc.month, data->utc.month_day,
            data->utc.hour, data->utc.minute, data->utc.millisecond / 1000,
            data->utc.millisecond % 1000);

    struct rtc_time time = {
        .tm_hour = data->utc.hour,
        .tm_isdst = 0,
        .tm_mday = data->utc.month_day,
        .tm_min = data->utc.minute,
        .tm_mon = data->utc.month,
        .tm_nsec = (data->utc.millisecond % 1000) * 1000000,
        .tm_sec = data->utc.millisecond / 1000,
        .tm_year = data->utc.century_year,
    };

    if ((errno = rtc_set_time(rtc_dev, &time)) != 0) {
        LOG_ERR("Failed to update the RTC time. Error: %d", errno);
        return;
    }

    if ((errno = gpio_pin_set_dt(&gnss_disable, 1)) != 0) {
        LOG_ERR("Failed to disable the GNSS module. Error: %d", errno);
    }

    synced_rtc_with_gps = true;
    LOG_INF("Aligned the RTC to GNSS.");
}

GNSS_DATA_CALLBACK_DEFINE(gnss_dev, gnss_data_callback);

trutime_t trutime_init(trutime_t trutime) {
    int errno;

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

    LOG_INF("Initialized trutime.");
    return trutime;
}

int trutime_get_utc(trutime_t trutime, struct tm* out) {
    int errno;

    if (!synced_rtc_with_gps) {
        return -ENODATA;
    }

    struct rtc_time rtctime = { 0 };
    if ((errno = rtc_get_time(rtc_dev, &rtctime)) != 0) {
        LOG_ERR("Failed to perform rtc_get_time. Error = %d", errno);
        return errno;
    }

    struct tm *tm_time = rtc_time_to_tm(&rtctime);
    memcpy(out, tm_time, sizeof(struct tm));
    return 0;
}
