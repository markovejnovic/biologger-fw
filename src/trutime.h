/**
 * @brief Module responsible for providing the true UTC time.
 */
#ifndef TRUTIME_H
#define TRUTIME_H

#include "observer.h"
#include "zephyr/drivers/rtc.h"
#include <time.h>
#include <stdbool.h>

/**
 * @brief The data structure representing the trutime module.
 */
struct trutime_data {};

typedef struct trutime_data* trutime_t;

/**
 * @brief Initialize the module.
 *
 * @param [in] t The pointer to the trutime data structure.
 * @param [in] o The application observer.
 *
 * @return The trutime object.
 */
trutime_t trutime_init(struct trutime_data* t, observer_t o);

/**
 * @brief Query the true UTC time.
 *
 * @param [in] t The trutime object.
 * @param [out] time The pointer to the time structure to write to.
 *
 * @return The error code if any error occurs.
 */
int trutime_get_utc(trutime_t t, struct rtc_time* time);

/**
 * @brief Return the total count of milliseconds relative to an instant.
 *
 * @param [in] t The trutime object.
 * @param [in] since The instant to return the total count of millis from.
 *
 * @return Negative errno on error or a positive number indicating the number
 *         of millis.
 */
long long trutime_millis_since(trutime_t t, struct rtc_time* since);

/**
 * @brief Query whether the UTC synchronized time is available.
 * @param [in] t The trutime object.
 * @return Whether trutime is currently synchronized and available.
 */
bool trutime_is_available(trutime_t t);

/**
 * @brief Declare a trutime object.
 * @param [in] name The name of the trutime object.
 */
#define TRUTIME_DECL(name) \
    static struct trutime_data trutime_##name##_data

/**
 * @brief Initialize a trutime object.
 *
 * @param [in] name The name of the trutime object.
 * @param [in] observer The application observer.h object.
 *
 * @return The trutime object.
 */
#define TRUTIME_INIT(name, observer) \
    trutime_init(&trutime_##name##_data, observer)

#endif /* TRUTIME_H */
