/**
 * @brief The storage module is responsible for managing the state of the SD
 *        card as well as USB storage.
 *
 * @details
 * This module manages the SD card by spawning a new thread in which it will
 * periodically poll the SD card for its state. The state of the SD card
 * informs the module as to which state it itself should be in.
 *
 * Due to limitations in Zephyr (and my own lack of time/willpower), the SD
 * card cannot be hotplugged.
 *
 * This module is however designed to survive cases of data loss due to
 * hotplugging.
 */
#ifndef STORAGE_H
#define STORAGE_H

#include <time.h>
#include <zephyr/sys/slist.h>
#include "observer.h"

typedef struct experiment* experiment_t;
typedef struct experiment_row* experiment_row_t;

typedef struct storage* storage_t;

/**
 * @brief Initialize the storage module.
 * @param [in] observer The observer module.
 */
storage_t storage_init(observer_t observer);

/**
 * @brief Neatly close the storage module.
 */
int storage_close(storage_t);

/**
 * @brief Create a new transaction into permanent storage. Transactions are
 *        units of operating between which a flush to disk will occur.
 *
 * @param [in] storage The storage module.
 * @param [in] start_time The time when the experiment was started.
 * @return An error code if any.
 *
 * @warning storage_wait_until_available must pass before this can be called.
 */
int storage_transaction(storage_t storage, const struct tm* start_time);

/**
 * @brief Write a row to the currently open file.
 * @param [in] storage The storage module.
 * @param [in] row The row to write.
 * @param [in] row_sz The number of bytes in the row.
 * @note You should not have a trailing newline.
 * @return An error code if any.
 *
 * @warning storage_wait_until_available must pass before this can be called.
 */
int storage_write_row(storage_t storage, const char* row, size_t row_sz);

/**
 * @brief Close the currently open file.
 * @param [in] storage The storage module.
 *
 * @note You do not normally need to call this as storage_close will call it
 *       for you.
 *
 * @warning storage_wait_until_available must pass before this can be called.
 */
int storage_close_file(storage_t storage);

/**
 * @brief Sleep the current thread of execution until storage is available.
 * @param [in] storage The storage module.
 */
void storage_wait_until_available(storage_t storage);

/**
 * @brief Flush any cached state to disk.
 * @param [in] storage The storage module.
 */
int storage_flush(storage_t storage);

#endif // STORAGE_H
