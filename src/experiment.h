/**
 * @note This module is NOT thread safe.
 * @see struct experiment
 *
 * Example:
 * @code{.c}
 * // Initialize the experiment.
 * struct experiment* experiment = experiment_init(storage);
 *
 * // Add columns to the experiment.
 * experiment_add_column(experiment, "Windspeed X", "m/s");
 * experiment_add_column(experiment, "Windspeed Y", "m/s");
 *
 * // Perform some measurements, for the sake of the example.
 * const double windspeed_x = windspeed_x_sample_get();
 * const double windspeed_y = windspeed_y_sample_get();
 *
 * // Create a new row.
 * struct experiment_row* row = experiment_row_new();
 * experiment_row_add_value(row, windspeed_x);
 * experiment_row_add_value(row, windspeed_y);
 * experiment_push_row(experiment, row);
 *
 * // ...
 * // When the application is destroyed, ensure you destroy the experiment to
 * // ensure it is fully flushed.
 * experiment_flush(experiment);
 * @endcode
 */
#ifndef EXPERIMENT_H
#define EXPERIMENT_H

#include <zephyr/sys/slist.h>
#include <time.h>

#define MAX_EXPERIMENT_COLS (128)

// Forward declaration required in experiment_init.
typedef struct storage* storage_t;

/**
 * @brief Represents an ongoing experiment.
 *
 * @details
 * The experiment is defined as a set of samples in monotonically increasing
 * time, offset from a start time. Data is a matrix of columns representing the
 * axes of the experiment and rows representing increasing values in time.
 *
 * This structure encodes this meaning in two linked lists. The columns
 * linked list is a set of axes the user defines via the experiment_add_column
 * function. The rows are represented as a linked list of rows. A new row is
 * added with experiment_push_row.
 */
struct experiment {
    struct tm start_time_utc; /*!< The experiment start time. */

    sys_slist_t columns; /*!< The columns linked list. */
    size_t column_count; /*!< The total number of columns */
    bool columns_flushed; /*!< Whether the columns have been flushed. */

    sys_slist_t rows; /*!< The rows linked list. */
    size_t rows_count; /*!< The total number of rows. */

    storage_t storage; /*!< A reference to the application storage. */
};

struct experiment_caption {
    sys_snode_t node;
    char* column_name;
    char* unit;
};

struct experiment_row {
    sys_snode_t node;
    double values[MAX_EXPERIMENT_COLS];
};

/**
 * Initialize and heap allocate a new experiment object.
 *
 * @param [in] storage A storage.h object to write data into.
 * @param [in] time A 
 */
struct experiment* experiment_init(storage_t storage);

void experiment_free(struct experiment*);

/**
 * Allocate and add a new column to an already open experiment.
 *
 * @param [in] experiment The experiment to add a column to.
 * @param [in] name The new column name.
 * @param [in] units The column units.
 *
 * @return 0 On a successful addition or -ENOMEM if the device is OOM.
 *
 * @warning It is illegal to add a column after the first row has been added.
 */
int experiment_add_column(struct experiment* experiment,
                          const char* name, const char* units);

/**
 * @brief Create a new heap-allocated experiment row.
 */
struct experiment_row* experiment_row_new(void);

/**
 * @brief Append a new value to the experiment.
 *
 * @param [in] row The experiment row to add values into.
 * @param [in] value The new value to push
 *
 * @note Experiment values must be pushed into the row in the same order that
 *       the columns were defined.
 */
int experiment_row_add_value(struct experiment_row* row, double value);

/**
 * @brief Append a new row to an already-open experiment.
 *
 * @param [in] experiment The experiment to add a new row to.
 * @param [in] row A pointer to a heap-allocated row. This row must already be
 *                 initialized. This function will take ownership of the row.
 *
 * @warning data MUST be an array of floats containing as many elements as
 *          columns have been defined. This function will not perform said
 *          validation.
 *
 * @return 0 on success, -ENOMEM if the device is out of memory.
 */
int experiment_push_row(struct experiment* experiment,
                        struct experiment_row* data);

/**
 * @brief Flush the experiment down into permanent storage. The object is still
 *        re-usable after this function is called.
 *
 * @param [in] The exeperiment to flush.
 */
int experiment_flush(struct experiment* experiment);

#endif /* EXPERIMENT_H */
