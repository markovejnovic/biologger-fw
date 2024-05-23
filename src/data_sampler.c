#include "data_sampler.h"
#include "zephyr/arch/arch_interface.h"
#include "zephyr/kernel.h"
#include "zephyr/sys_clock.h"
#include <zephyr/kernel/thread.h>

struct dsampler {
    struct k_thread thread_data;
    const struct dsampler_config config;

    k_tid_t thread_id;
};

static void dsmain(void* p1, void* _, void* __)
{
    const dsampler_t ds = (dsampler_t)p1;
    const k_timeout_t sample_period = ds->config.sample_period;

    while (1)
    {
        k_sleep(sample_period);
    }
}

void dsampler_begin(dsampler_t d) {
    d->thread_id = k_thread_create(
        &d->thread_data,
        d->config.thread_stack, d->config.thread_stack_size,
        dsmain,
        d, NULL, NULL,
        d->config.scheduling_prio,
        0,
        K_NO_WAIT
    );
}
