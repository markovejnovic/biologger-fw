/*
 * Copyright (c) 2024 Marko Vejnovic
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/arch_interface.h>
#include <zephyr/sys_clock.h>

typedef struct dsampler* dsampler_t;

struct dsampler_config {
    int scheduling_prio;

    k_thread_stack_t* thread_stack;
    size_t thread_stack_size;

    k_timeout_t sample_period;
};

dsampler_t dsampler_create(const struct dsampler_config config);

void dsampler_begin(dsampler_t);
