#include "tsic_506.h"
#include <zephyr/sys/__assert.h>
#include <sys/errno.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>

LOG_MODULE_REGISTER(TH02, CONFIG_SENSOR_LOG_LEVEL);

#define LOWER_TEMPERATURE_LIMIT -10
#define HIGHER_TEMPERATURE_LIMIT 60

static int tsic_506f_channel_get(const struct device *dev,
                                 enum sensor_channel chan,
                                 struct sensor_value *val)
{
    struct tsic_506f_data *data = dev->data;

    __ASSERT_NO_MSG(chann == SENSOR_CHAN_AMBIENT_TEMP);

    if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
        return -ENOTSUP;
    }

    // TODO(markovejnovic): Pack val.
}

static void tsic_interrupt(const struct device *dev, struct gpio_callback *cb,
                           uint32_t pins)
{
    struct tsic_506f_data *data = dev->data;
}

static int tsic_506f_init(const struct device *dev)
{
    const struct tsic_506f_config *cfg = dev->config;
    struct tsic_506f_data *data = dev->data;

    int errnum;

    if (!gpio_is_ready_dt(&cfg->gpio)) {
        LOG_ERR("GPIO is not ready. Cannot initialize.");
        return -ENODEV;
    }

    if ((errnum = gpio_pin_configure_dt(&cfg->gpio, GPIO_INPUT)) != 0) {
        LOG_ERR("Failed to assign GPIO %s:%d to be input. Err: %d",
                cfg->gpio.port->name, cfg->gpio.pin, errnum);
        return errnum;
    }

    if ((errnum = gpio_pin_interrupt_configure_dt(&cfg->gpio,
                                                  GPIO_INT_EDGE_BOTH)) != 0) {
        LOG_ERR("Failed to setup a GPIO interrupt on %s:%d. Err: %d",
                cfg->gpio.port->name, cfg->gpio.pin, errnum);
        return errnum;
    }

    gpio_init_callback(&data->interrupt_cb_data, tsic_interrupt,
                       BIT(cfg->gpio.pin));
    if ((errnum = gpio_add_callback(cfg->gpio.port,
                                    &data->interrupt_cb_data)) != 0) {
        LOG_ERR("Failed to add the callback for %s:%d. Err: %d",
                cfg->gpio.port->name, cfg->gpio.pin, errnum);
        return errnum;
    }

    return 0;
}

static const struct sensor_driver_api tsic_506f_api = {
    .sample_fetch = tsic_506f_sample_fetch,
    .channel_get = tsic_506f_channel_get,
};

#define TSIC_506F_DEFINE(inst)                                                 \
    static struct tsic_506f_data tsic_506f_data_##inst;                        \
                                                                               \
    static const struct tsic_506f_config tsic_506f_config_##inst = {           \
        .gpio = GPIO_DT_SPEC_INST_GET(inst),                                   \
    };                                                                         \
                                                                               \
    SENSOR_DEVICE_DT_INST_DEFINE(inst, tsic_506f_init, NULL,                   \
                                 &tsic_506f_data_##inst,                       \
                                 &tsic_506f_config_##inst,                     \
                                 POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,     \
                                 &tsic_506f_driver_api);

DT_INST_FOREACH_STATUS_OKAY(TSIC_506F_DEFINE)
