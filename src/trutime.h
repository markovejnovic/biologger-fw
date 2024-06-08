#ifndef _TRUTIME_H
#define _TRUTIME_H

#include "observer.h"
#include <time.h>
#include <stdbool.h>

struct trutime_data {
};
typedef struct trutime_data* trutime_t;

trutime_t trutime_init(trutime_t, observer_t);
int trutime_get_utc(trutime_t, struct tm*);

#define TRUTIME_DECL(name) \
    static struct trutime_data trutime_##name##_data

#define TRUTIME_INIT(name, observer) \
    trutime_init(&trutime_##name##_data, observer)

#endif // _TRUTIME_H_
