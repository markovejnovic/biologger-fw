/**
 * @brief Tracks the state of the application and drives the LEDs indicating
 *        the current state of the application.
 */
#ifndef OBSERVER_H
#define OBSERVER_H

#include <zephyr/kernel.h>

/**
 * @brief The state the application is in.
 *
 * The enumeration values defined here dictate what state the application is
 * currently in and directly correspond to the LED behavior.
 */
enum app_state {
    /*!< The application has faulted due to an unspecified reason.
     * The LED will be enabled.
     */
    APP_STATE_DEVICE_FAULT,
    /*!< The application is waiting for a time signal to be acquired.
     * The LED will blink slowly.
     */
    APP_STATE_WAITING_FOR_TIMESYNC,
    /*!< The application is sampling devices.
     * The LED will blink every 500ms.
     */
    APP_STATE_SAMPLING,
};

/**
 * @brief Events that may be raised by the application.
 */
enum observer_flag {
    /*!< The application has determined that no storage media is currently
     * available.
     */
    OBSERVER_FLAG_NO_DISK = 1,

    /*!< The application has determined that no valid GPS clock is currently
     * available.
     */
    OBSERVER_FLAG_NO_GPS_CLOCK = 2,

    /*!< The application has a driver that did not initialize correctly.
     *
     */
    OBSERVER_FLAG_DRIVER_MIA = 3,

    OBSERVER_FLAG_COUNT,
};

typedef struct {
    enum app_state app_state;
    bool flag_states[OBSERVER_FLAG_COUNT];
} observer_data;

typedef observer_data* observer_t;

observer_t observer_start(observer_t);

#define OBSERVER_INIT(name) \
    observer_start(&observer_##name##_data)

#define OBSERVER_DECL(name) \
    observer_data observer_##name##_data;

/**
 * @brief Notify the observer that an event has occurred.
 *
 * @param [in] observer The observer object.
 * @param [in] flag The relevant event flag.
 */
void observer_flag_raise(observer_t, enum observer_flag flag);

/**
 * @brief Notify the observer that an event has dropped.
 *
 * @param [in] observer The observer object.
 * @param [in] flag The relevant event flag.
 */
void observer_flag_lower(observer_t, enum observer_flag flag);

/**
 * @brief Wait until all the flags are are of the specified state.
 *
 * @param [in] observer The observer object.
 * @param [in] flags The flags to all wait for.
 * @param [in] state The state you wish the flags to be in.
 * @param [in] timeout The maximum timeout PER flag.
 *
 * @return int zero if all flags were given.
 */
int observer_wait_for_all(observer_t observer, enum observer_flag* flags,
                          bool state, k_timeout_t timeout);

#endif /* OBSERVER_H */
