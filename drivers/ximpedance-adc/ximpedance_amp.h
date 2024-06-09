#ifndef XIMPEDANCE_AMP_H
#define XIMPEDANCE_AMP_H

#include <zephyr/drivers/adc.h>
#include <zephyr/device.h>

struct ximpedance_amp_config {
    struct adc_dt_spec adc0;
    struct adc_dt_spec adc1;
    struct adc_dt_spec adc2;
    struct adc_dt_spec adc3;
};

struct _ximpedance_adc_entry {
    const struct adc_dt_spec* dt;
    uint16_t buffer;
    struct adc_sequence sequence;
};

struct ximpedance_amp_data {
    struct _ximpedance_adc_entry all_adcs[4];
};

#endif /* XIMPEDANCE_AMP_H */
