#include "storage.h"
#include <sys/errno.h>
#include <zephyr/sys/slist.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(storage);

struct storage {
    size_t open_objects;
};

struct experiment {
    storage_t storage;
};

storage_t storage_init(void) {
    storage_t storage = k_malloc(sizeof(struct storage));

    storage->open_objects = 0;

    return storage;
}

int storage_close(storage_t storage) {
    if (storage == NULL) {
        return 0;
    }

    if (storage->open_objects != 0) {
        return -EMFILE;
    }

    k_free(storage);
    return 0;
}

stored_experiment_t storage_stored_experiment_new(storage_t storage) {
    if (storage == NULL) {
        return NULL;
    }

    storage->open_objects++;

    stored_experiment_t experiment = k_malloc(sizeof(struct experiment));
    experiment->storage = storage;

    return experiment;
}

int storage_stored_experiment_close(stored_experiment_t experiment) {
    if (experiment == NULL) {
        return 0;
    }
    
    experiment->storage--;

    k_free(experiment);
    return 0;
}
