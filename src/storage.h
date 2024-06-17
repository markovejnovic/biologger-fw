#ifndef STORAGE_H
#define STORAGE_H

#include "observer.h"
#include <time.h>
#include <zephyr/sys/slist.h>

typedef struct experiment* experiment_t;
typedef struct experiment_row* experiment_row_t;

typedef struct storage* storage_t;
typedef struct experiment* stored_experiment_t;

storage_t storage_init(observer_t observer);
int storage_close(storage_t);

int storage_open_file(storage_t storage, struct tm* start_time);
int storage_write_row(storage_t storage, const char* row);
int close_file(storage_t storage);

#endif // STORAGE_H
