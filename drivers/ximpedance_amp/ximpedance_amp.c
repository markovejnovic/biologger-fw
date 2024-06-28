#include "ximpedance_amp.h"
#include "v2i_ximpedance10x_lut.h"
#include "v2i_ximpedance22x_lut.h"
#include <sys/errno.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_DECLARE(ximpedance_amp);

inline static int32_t get_nanoamps_for_microvolts(size_t adc_idx, int32_t uv) {
    switch (adc_idx) {
        case 0:
        case 1:
            return v2i_ximpedance22x_lut_get_nanoamps_from_microvolts(uv);
        case 2:
        case 3:
            return v2i_ximpedance10x_lut_get_nanoamps_from_microvolts(uv);
    }

    return 0;
}

static int ximpedance_amp_sample_fetch(const struct device* dev,
                                       enum sensor_channel chan) {
    struct ximpedance_amp_data* data = dev->data;

    if (chan != SENSOR_CHAN_ALL) {
        LOG_ERR("The Ximpedance driver requires you poll the whole sensor.");
        return -ENOTSUP;
    }

    // We cannot simply sample the target channels due to how the ADS1X1X
    // driver works. We need to reconfigure the channels because we explicitly
    // need to set input_positive. See the Zephyr
    // adc_ads1x1x.c:ads1x1x_channel_setup driver for more details.
    int cum_error = 0;
    for (size_t channel = 0; channel < XIMPEDANCE_AMP_V1_CHANNELS; channel++) {
        int err = 0;
        data->adc_spec.channel_cfg.input_positive = channel;
        if ((err = adc_channel_setup_dt(&data->adc_spec)) != 0) {
            LOG_ERR("Failed to setup ADC channel for transimpedance channel %d "
                    "(%d).", channel, err);
            goto continue_loop;
        }

        // Configure the sequence
        uint16_t buf;
        struct adc_sequence sequence = {
            .buffer = &buf,
            .buffer_size = sizeof(buf),
        };
        if ((err = adc_sequence_init_dt(&data->adc_spec, &sequence)) != 0) {
            LOG_ERR("Failed to init ADC sequence for transimpedance channel %d "
                    "(%d).", channel, err);
            goto continue_loop;
        }

        int32_t val_mv = (int32_t)buf;
        if ((err = adc_raw_to_millivolts_dt(&data->adc_spec, &val_mv)) != 0) {
            LOG_ERR("Failed to convert ADC raw to mV for transimpedance channel"
                    "%d (%d).", channel, err);
            goto continue_loop;
        }

        // Now we need to follow the IV curve to compute the current we just
        // sampled.
        data->sampled_nanoamps[channel] =
            get_nanoamps_for_microvolts(channel, val_mv);

continue_loop:
        cum_error = MIN(cum_error, err);
    }

    return cum_error;
}

static int ximpedance_amp_channel_get(const struct device *dev,
                                      enum sensor_channel chan,
                                      struct sensor_value *val) {
    struct ximpedance_amp_data* data = dev->data;

    size_t adc_index;
    switch ((int)chan) {
        case XIMPEDANCE_CHAN_22KX_MILLIAMPS_1:
            adc_index = 0;
            break;

        case XIMPEDANCE_CHAN_22KX_MILLIAMPS_2:
            adc_index = 1;
            break;

        case XIMPEDANCE_CHAN_10KX_MILLIAMPS_1:
            adc_index = 2;
            break;

        case XIMPEDANCE_CHAN_10KX_MILLIAMPS_2:
            adc_index = 3;
            break;

        default:
            LOG_ERR("The Ximpedance requires chan is one of 0-3.");
            return -ENOTSUP;
    }

    int32_t nanoamps = data->sampled_nanoamps[adc_index];
    val->val1 = nanoamps / (1000 * 1000);
    val->val2 = nanoamps % (1000 * 1000);

    return 0;
}

static const struct sensor_driver_api ximpedance_amp_driver_api = {
    .sample_fetch = ximpedance_amp_sample_fetch,
    .channel_get = ximpedance_amp_channel_get,
};

static int ximpedance_amp_init(const struct device *dev) {
    struct ximpedance_amp_data* data = dev->data;
    int err;

    if (!adc_is_ready_dt(&data->adc_spec)) {
        LOG_ERR("The transimpedance amplifier did not initialize.");
        return -ENODEV;
    }

    // Perform a preliminary channel config as a sanity check.
    if ((err = adc_channel_setup_dt(&data->adc_spec)) != 0) {
        LOG_ERR("Preliminary failed to setup ADC channel (%d).", err);
    }

    return 0;
}

#define XIMPEDANCE_AMP_DEFINE(inst)                                            \
    static struct ximpedance_amp_data ximpedance_amp_data_##inst = {           \
        .adc_spec = ADC_DT_SPEC_GET_BY_NAME(DT_NODELABEL(ximpedance_amp), a0), \
    };                                                                         \
    static const struct ximpedance_amp_config ximpedance_amp_config_##inst;    \
                                                                               \
    SENSOR_DEVICE_DT_INST_DEFINE(inst, ximpedance_amp_init, NULL,              \
                                 &ximpedance_amp_data_##inst,                  \
                                 &ximpedance_amp_config_##inst,                \
                                 POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,     \
                                 &ximpedance_amp_api);


DT_INST_FOREACH_STATUS_OKAY(XIMPEDANCE_AMP_DEFINE)
