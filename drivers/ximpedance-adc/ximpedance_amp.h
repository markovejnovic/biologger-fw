#ifndef XIMPEDANCE_AMP_H
#define XIMPEDANCE_AMP_H

#include <zephyr/drivers/adc.h>
#include <zephyr/device.h>

#define XIMPEDANCE_AMP_V1_CHANNELS (4)

struct ximpedance_amp_config {
};

struct ximpedance_amp_data {
    // Since we mutate the configuration of the ADC during run-time (to pick
    // the ADC mux pinout) we pack it as data rather than config.
    struct adc_dt_spec adc_spec;

    int32_t sampled_microamps[XIMPEDANCE_AMP_V1_CHANNELS];
};

#endif /* XIMPEDANCE_AMP_H */
