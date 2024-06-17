#include "trisonica_mini.h"

#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>

static int trisonica_mini_sample_fetch(const struct device* dev,
                                       enum sensor_channel chan) {
}

static int trisonica_mini_channel_get(const struct device *dev,
                                      enum sensor_channel chan,
                                      struct sensor_value *val) {
}

static const struct sensor_driver_api trisonica_mini_api = {
    .sample_fetch = trisonica_mini_sample_fetch,
    .channel_get = trisonica_mini_channel_get,
};

static int trisonica_mini_init(const struct device *dev) {
}

#define TRISONICA_MINI_DEFINE(inst)                                            \
    static struct trisonica_mini_data trisonica_mini_data_##inst = { 0 };      \
    SENSOR_DEVICE_DT_INST_DEFINE(inst, trisonica_mini_init, NULL,              \
                                 &trisonica_mini_data_##inst,                  \
                                 &trisonica_mini_config_##inst,                \
                                 POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,     \
                                 &trisonica_mini_api);

DT_INST_FOREACH_STATUS_OKAY(TRISONICA_MINI_DEFINE)
