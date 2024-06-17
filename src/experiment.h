/**
 * @note This module is NOT thread safe.
 */
#include <zephyr/sys/slist.h>
#include <time.h>

struct experiment {
    struct timespec start_time_utc;

    sys_slist_t columns;
    size_t column_count;

    sys_slist_t rows;
};

struct experiment_caption {
    sys_snode_t node;
    char* column_name;
    char* unit;
};

struct experiment_row {
    sys_snode_t node;
    float* row;
};

/**
 * Initialize and heap allocate a new experiment object.
 */
struct experiment* experiment_init(void);

void experiment_free(struct experiment*);

/**
 * Allocate and add a new column to an already open experiment.
 *
 * @param [in] experiment The experiment to add a column to.
 * @param [in] name The new column name.
 * @param [in] units The column units.
 *
 * @return 0 On a successful addition or -ENOMEM if the device is OOM.
 */
int experiment_add_column(struct experiment* experiment,
                          const char* name, const char* units);

/**
 * @brief Create a new heap-allocated experiment row.
 */
struct experiment* experiment_row_new(void);

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
