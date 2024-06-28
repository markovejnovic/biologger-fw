#include "experiment.h"
#include "storage.h"
#include "trutime.h"
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/slist.h>

#define EXPERIMENT_AUTO_FLUSH_THRESHOLD (10)

#define MAX_CELL_WIDTH (48)
#define MAX_COL_NAME_LEN (32)
#define MAX_COL_UNIT_LEN (MAX_CELL_WIDTH - (MAX_COL_NAME_LEN))

// Add +1 here because of the comma in the "%s," format string.
#define MAX_ROW_STR_LEN ((MAX_CELL_WIDTH + 1) * MAX_EXPERIMENT_COLS)

static char row_str_buf[MAX_ROW_STR_LEN];

LOG_MODULE_REGISTER(experiment);

static void free_caption(struct experiment_caption* capt) {
    k_free(capt->column_name);
    k_free(capt->unit);
    k_free(capt);
}

static void free_row(struct experiment_row* row) {
    k_free(row);
}

static void free_columns(struct experiment* exp) {
    // Only need to free the columns if they haven't been flushed. Otherwise,
    // they are eagerly freed during the flush.
    if (!exp->columns_flushed) {
        struct experiment_caption * entry, * next;
        SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&exp->columns, entry, next, node) {
            free_caption(entry);
            sys_slist_remove(&exp->columns, NULL, &entry->node);
        }
    }
}

/**
 * @brief Format the row of an experiment into CSV form.
 */
static struct strv format_row(
    char* buf,
    struct experiment_row* exp
) {
    char* write_buf = buf;

    // Write the relative timestamp.
    write_buf += snprintk(write_buf, MAX_CELL_WIDTH, "%llu,",
                          exp->millis_since_start);

    // Write every single row value.
    for (size_t i = 0; i < exp->value_count; i++) {
        const double value = exp->values[i];
        write_buf += snprintk(write_buf, MAX_CELL_WIDTH, "%10.10f,", value);
    }

    // Null terminate the string, overriding the last comma.
    *(--write_buf) = 0;

    return (struct strv) {
        .len = write_buf - buf,
        .str = buf,
    };
}

/**
 * @brief Commit all the columns to the database.
 *
 * @todo(markovejnovic) This is some really dangerous code right here. There's
 * a ton of malloc that looks pretty scary.
 */
static void flush_columns(struct experiment* experiment) {
    int err;
// The buffer size is large enough to match the whole column name, the units,
// the 4 characters " [],". The format is "%s [%s],..." hence the 4 extra
// characters.
#define MAX_COL_STR_LEN (MAX_COL_NAME_LEN + MAX_COL_UNIT_LEN + 4)
#define MAX_TOTAL_COL_STR_LEN (MAX_COL_STR_LEN * MAX_EXPERIMENT_COLS)

    // Add +1 for '\0'.
    char* column_str = k_malloc(MAX_TOTAL_COL_STR_LEN + 1);
    if (column_str == NULL) {
        LOG_ERR("Failed to initialize enough memory for column_str");
        return;
    }
    char* work_ptr = column_str;

    // Add the timestamp column.
    work_ptr += snprintk(work_ptr, MAX_COL_STR_LEN, "Timestamp [ms],");

    // For each single column, convert it to a string and append it to the
    // "long" string representing the columns.
    struct experiment_caption * entry;
    SYS_SLIST_FOR_EACH_CONTAINER(&experiment->columns, entry, node) {
        work_ptr += snprintk(work_ptr, MAX_COL_STR_LEN, "%s [%s],",
                             entry->column_name, entry->unit);
    }

    // Move the pointer back by one to delete the last ','. Null-terminate the
    // string.
    *(--work_ptr) = 0;

    const size_t str_len = work_ptr - column_str;
    if ((err = storage_write_row(experiment->storage,
                                 (struct strv) { column_str, str_len })) != 0) {
        LOG_ERR("Failed to write to storage (%d)", err);
    }
    k_free(column_str);

#undef MAX_COL_STR_LEN
#undef MAX_TOTAL_COL_STR_LEN
}

struct experiment* experiment_init(storage_t storage, trutime_t trutime) {
    struct experiment* exp = k_malloc(sizeof(struct experiment));
    if (exp == NULL) {
        LOG_ERR("Failed to initialize enough memory in experiment_init.");
        return NULL;
    }

    sys_slist_init(&exp->columns);
    exp->column_count = 0;
    exp->columns_flushed = false;

    sys_slist_init(&exp->rows);
    exp->rows_count = 0;

    exp->storage = storage;
    exp->trutime = trutime;

    // We need to open the required file.
    storage_wait_until_available(storage);

    // Seed the start time of the experiment.
    int err;
    if ((err = trutime_get_utc(trutime, &exp->start_time_utc)) != 0) {
        LOG_ERR("Could not fetch the true time to init the experiment (%d).",
                err);
    }
    storage_transaction(storage, (struct tm*)&exp->start_time_utc);

    return exp;
}

void experiment_free(struct experiment * exp) {
    experiment_flush(exp);
    free_columns(exp);

    k_free(exp);
}

int experiment_add_column(
    struct experiment *experiment,
    const char * name,
    const char * units
) {
    struct experiment_caption* node
        = k_malloc(sizeof(struct experiment_caption));
    if (node == NULL) {
        LOG_ERR("Failed to initialize enough memory for struct "
                "experiment_caption");
        return -ENOMEM;
    }

    const size_t name_len = strlen(name);
    node->column_name = k_malloc(name_len + 1);
    if (node->column_name == NULL) {
        k_free(node);
        LOG_ERR("Failed to initialize enough memory for node->column_name");
        return -ENOMEM;
    }
    strncpy(node->column_name, name, name_len + 1);

    const size_t unit_len = strlen(units);
    node->unit = k_malloc(unit_len + 1);
    if (node->unit == NULL) {
        k_free(node->column_name);
        k_free(node);
        LOG_ERR("Failed to initialize enough memory for node->unit");
        return -ENOMEM;
    }
    strncpy(node->unit, units, unit_len + 1);

    sys_slist_append(&experiment->columns, &node->node);
    experiment->column_count++;

    return 0;
}

struct experiment_row* experiment_row_new(
    unsigned long long millis_since_start
) {
    // Allocate values for the row.
    struct experiment_row* row = k_malloc(sizeof(struct experiment_row));
    if (row == NULL) {
        LOG_ERR("Could not k_malloc enough memory for an experiment_row");
        return NULL;
    }
    row->value_count = 0;
    row->millis_since_start = millis_since_start;
    return row;
}

int experiment_push_row(
    struct experiment* experiment,
    struct experiment_row* row
) {
    if (!experiment->columns_flushed) {
        flush_columns(experiment);

        // It is safe to deallocate the whole list now.
        free_columns(experiment);

        experiment->columns_flushed = true;
    }

    sys_slist_append(&experiment->rows, &row->node);
    experiment->rows_count++;

    if (experiment->rows_count >= EXPERIMENT_AUTO_FLUSH_THRESHOLD) {
        return experiment_flush(experiment);
    }

    return 0;
}

int experiment_flush(struct experiment* experiment) {
    int err = 0;

    // Write every single experiment into the storage.
    struct experiment_row * entry, * next;
    SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&experiment->rows, entry, next, node) {
        const struct strv row_str = format_row(row_str_buf, entry);

        // Attempt to write this to persistent storage.
        if ((err = storage_write_row(experiment->storage, row_str)) != 0) {
            LOG_ERR("Failed to push the experiment row.");
            err++;
        }

        // Regardless of whether that succeeded or failed, we still need to pop
        // this sample from the linked list otherwise we risk leaking memory.
        sys_slist_remove(&experiment->rows, NULL, &entry->node);
        experiment->rows_count--;

        free_row(entry);
    }

    return err;
}

int experiment_row_add_value(struct experiment_row *row, double value) {
    row->values[row->value_count++] = value;
    return 0;
}

const struct rtc_time* experiment_start_time(
    const struct experiment* experiment
) {
    return &experiment->start_time_utc;
}

struct strv experiment_row_format(struct experiment_row *row) {
    char* memory = k_malloc(MAX_ROW_STR_LEN);
    if unlikely(memory == NULL) {
        LOG_ERR("Failed to allocate memory. The system is now in an "
                "indeterminate state");
        return (struct strv){};
    }

    return format_row(memory, row);
}
