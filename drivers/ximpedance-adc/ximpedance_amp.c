#include "ximpedance_amp.h"
#include <sys/errno.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_DECLARE(ximpedance_amp);

static int ximpedance_amp_sample_fetch(const struct device* dev,
                                       enum sensor_channel chan) {
    if (chan != SENSOR_CHAN_ALL) {
        LOG_ERR("The Ximpedance driver requires you poll the whole sensor.");
        return -ENOTSUP;
    }

    int errnum;
    int carryover_errnum = 0;
    struct ximpedance_amp_data* data = dev->data;
    ARRAY_FOR_EACH_PTR(data->all_adcs, adc) {
        if ((errnum = adc_read_dt(adc->dt, &adc->sequence)) != 0) {
            LOG_ERR("Failed to read the value of %s (%d).",
                    adc->dt->dev->name, errnum);
        }
        carryover_errnum = errnum != 0 ? errnum : carryover_errnum;
    }

    return carryover_errnum;
}

static int ximpedance_amp_channel_get(const struct device *dev,
                                      enum sensor_channel chan,
                                      struct sensor_value *val) {
    struct ximpedance_amp_data* data = dev->data;

    size_t adc_index;
    switch (chan) {
        case 0:
        case 1:
        case 2:
        case 3:
            adc_index = (size_t)chan;
        default:
            LOG_ERR("The Ximpedance requires chan is one of 0-3.");
            return -ENOTSUP;
    }

    struct _ximpedance_adc_entry* adc = &data->all_adcs[adc_index];
    int errnum;

    int32_t val_mv = (int32_t)adc->buffer;
    if ((errnum = adc_raw_to_millivolts_dt(adc->dt, &val_mv)) != 0) {
        LOG_ERR("Failed to convert raw ADC value to mV for %s (%d)",
                adc->dt->dev->name, errnum);
        return errnum;
    }

    // TODO(markovejnovic): This conversion is just wrong.
    val->val1 = val_mv;
    return 0;
}

static const struct sensor_driver_api ximpedance_amp_driver_api = {
    .sample_fetch = ximpedance_amp_sample_fetch,
    .channel_get = ximpedance_amp_channel_get,
};

static int ximpedance_amp_init(const struct device *dev) {
    const struct ximpedance_amp_config *cfg = dev->config;
    struct ximpedance_amp_data* data = dev->data;
    int errnum;

    data->all_adcs[0] = (struct _ximpedance_adc_entry){ .dt = &cfg->adc0 };
    data->all_adcs[1] = (struct _ximpedance_adc_entry){ .dt = &cfg->adc1 };
    data->all_adcs[2] = (struct _ximpedance_adc_entry){ .dt = &cfg->adc2 };
    data->all_adcs[3] = (struct _ximpedance_adc_entry){ .dt = &cfg->adc3 };

    ARRAY_FOR_EACH_PTR(data->all_adcs, adc) {
        if (!adc_is_ready_dt(adc->dt)) {
            LOG_ERR("%s is not ready. Failing.", adc->dt->dev->name);
            return -ENODEV;
        }

        if ((errnum = adc_channel_setup_dt(adc->dt)) != 0) {
            LOG_ERR("Failed to setup the ADC channel for %s (%d).",
                    adc->dt->dev->name, errnum);
            return errnum;
        }

        adc->sequence = (struct adc_sequence){
            .buffer = &adc->buffer,
            .buffer_size = sizeof(adc->buffer),
        };

        if ((errnum = adc_sequence_init_dt(adc->dt, &adc->sequence)) != 0) {
            LOG_ERR("Failed to initialize the sequence for %s (%d)",
                    adc->dt->dev->name, errnum);
        }
    }

    return 0;
}

#define XIMPEDANCE_AMP_DEFINE(inst)                                            \
    static struct ximpedance_amp_data ximpedance_amp_data_##inst;              \
    static const struct ximpedance_amp_config ximpedance_amp_config_##inst = { \
        .adc0 = ADC_DT_SPEC_GET_BY_NAME(DT_NODELABEL(ximpedance_amp_adc), a0), \
        .adc1 = ADC_DT_SPEC_GET_BY_NAME(DT_NODELABEL(ximpedance_amp_adc), a1), \
        .adc2 = ADC_DT_SPEC_GET_BY_NAME(DT_NODELABEL(ximpedance_amp_adc), a2), \
        .adc3 = ADC_DT_SPEC_GET_BY_NAME(DT_NODELABEL(ximpedance_amp_adc), a3), \
    };                                                                         \
                                                                               \
    SENSOR_DEVICE_DT_INST_DEFINE(inst, ximpedance_amp_init, NULL,              \
                                 &ximpedance_amp_data_##inst,                  \
                                 &ximpedance_amp_config_##inst,                \
                                 POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,     \
                                 &ximpedance_amp_api);


DT_INST_FOREACH_STATUS_OKAY(XIMPEDANCE_AMP_DEFINE)
