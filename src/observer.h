#ifndef OBSERVER_H
#define OBSERVER_H

#include <zephyr/kernel.h>

typedef enum {
    OBSERVER_STATE_OPERATING_NORMALLY,
    OBSERVER_STATE_FAIL_INIT,
    OBSERVER_STATE_INITIALIZING,

    OBSERVER_STATE_WAITING_FOR_DISK,
} observer_state;

enum observer_flag {
    OBESRVER_FLAG_NO_DISK,
    OBSERVER_FLAG_NO_GPS_CLOCK,
};

typedef struct {
    _Atomic observer_state current_state;
} observer_data;

typedef observer_data* observer_t;

observer_t observer_start(observer_t);
void observer_set_state(observer_t, observer_state);

#define OBSERVER_INIT(name) \
    observer_start(&observer_##name##_data)

#define OBSERVER_DECL(name) \
    observer_data observer_##name##_data;

void observer_flag_raise(observer_t, enum observer_flag);
void observer_flag_lower(observer_t, enum observer_flag);

#endif // OBSERVER_H
