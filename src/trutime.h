/**
 * @brief Module responsible for providing the true UTC time.
 */
#ifndef TRUTIME_H
#define TRUTIME_H

#include "observer.h"
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
int trutime_get_utc(trutime_t t, struct tm* time);

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
