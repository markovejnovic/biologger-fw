#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>

#define DT_DRV_COMPAT licor,li_550
#define LI550_INIT_PRIORITY 41

struct li550_data {
};

struct li550_conf {
};

static const struct li550_api api = {
};

#define DEFINE_LI550(inst) \
    static struct li550_data li550_data_##inst = { \
    }; \
    static struct li550_conf li550_conf_##inst = { \
    }; \
    DEVICE_DT_INST_DEFINE(inst, init_li550, NULL, \
                          &li550_data_##inst, &li550_conf_##inst, \
                          POST_KERNEL, LI550_INIT_PRIORITY, &api);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_LI550);
