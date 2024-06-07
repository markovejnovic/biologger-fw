#include <zephyr/kernel.h>

typedef enum {
    OBSERVER_STATE_FAIL_INIT,
    OBSERVER_STATE_OPERATING_NORMALLY,
} observer_state;

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
