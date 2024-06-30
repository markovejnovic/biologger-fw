#ifndef XIMPEDANCE_AMP_H
#define XIMPEDANCE_AMP_H

#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/util.h>

#define XIMPEDANCE_AMP_V1_CHANNELS (4)
#define XIMPEDANCE_AMP_22K_GAIN_CHANNELS { 0, 1 }
#define XIMPEDANCE_AMP_10K_GAIN_CHANNELS { 2, 3 }

enum ximpedance_amp_sensor_channel {
    XIMPEDANCE_CHAN_22KX_MILLIAMPS_1 = SENSOR_CHAN_PRIV_START,
    XIMPEDANCE_CHAN_22KX_MILLIAMPS_2,
    XIMPEDANCE_CHAN_10KX_MILLIAMPS_1,
    XIMPEDANCE_CHAN_10KX_MILLIAMPS_2,
};

struct ximpedance_amp_config {
};

struct ximpedance_amp_data {
    // Since we mutate the configuration of the ADC during run-time (to pick
    // the ADC mux pinout) we pack it as data rather than config.
    struct adc_dt_spec adc_spec;

    int32_t sampled_nanoamps[XIMPEDANCE_AMP_V1_CHANNELS];
};

#endif /* XIMPEDANCE_AMP_H */
