#include "experiment.h"
#include "storage.h"
#include "zephyr/sys/printk.h"
#include "zephyr/sys/util.h"
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <zephyr/sys/slist.h>
#include <zephyr/logging/log.h>

#define EXPERIMENT_AUTO_FLUSH_THRESHOLD (10)

#define MAX_CELL_WIDTH (48)
#define MAX_COL_NAME_LEN (32)
#define MAX_COL_UNIT_LEN (MAX_CELL_WIDTH - (MAX_COL_NAME_LEN))

// Add +1 here because of the comma in the "%s," format string.
#define MAX_ROW_STR_LEN ((MAX_CELL_WIDTH + 1) * MAX_EXPERIMENT_COLS)

static char row_str_buf[MAX_ROW_STR_LEN];

LOG_MODULE_DECLARE(experiment);

static void free_caption(struct experiment_caption* capt) {
    free(capt->column_name);
    free(capt->unit);
    free(capt);
}

static void free_row(struct experiment_row* row) {
    free(row->values);
    free(row);
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
static char* format_row(char* buf, struct experiment_row* exp) {
    char* write_buf = buf;
    for (size_t i = 0; i < MAX_EXPERIMENT_COLS; i++) {
        const double value = exp->values[i];
        write_buf += snprintk(write_buf, MAX_CELL_WIDTH, "%10.10f,", value);
    }

    *(--write_buf) = 0;

    return buf;
}

/**
 * @brief Commit all the columns to the database.
 *
 * @todo(markovejnovic) This is some really dangerous code right here. There's
 * a ton of malloc that looks pretty scary.
 */
static void flush_columns(struct experiment* experiment) {
// The buffer size is large enough to match the whole column name, the units,
// the 4 characters " [],". The format is "%s [%s],..." hence the 4 extra
// characters.
#define MAX_COL_STR_LEN (MAX_COL_NAME_LEN + MAX_COL_UNIT_LEN + 4)
#define MAX_TOTAL_COL_STR_LEN (MAX_COL_STR_LEN * MAX_EXPERIMENT_COLS)

    // Add +1 for '\0'.
    char* column_str = malloc(MAX_TOTAL_COL_STR_LEN + 1);
    char* work_ptr = column_str;

    // For each single column, convert it to a string and append it to the
    // "long" string representing the columns.
    struct experiment_caption * entry;
    SYS_SLIST_FOR_EACH_CONTAINER(&experiment->columns, entry, node) {
        work_ptr += snprintk(work_ptr, MAX_COL_UNIT_LEN, "%s [%s],",
                             entry->column_name, entry->unit);
    }

    // Move the pointer back by one to delete the last ','. Null-terminate the
    // string.
    *(--work_ptr) = 0;

    storage_write_row(experiment->storage, column_str);
    free(column_str);

#undef MAX_COL_STR_LEN
#undef MAX_TOTAL_COL_STR_LEN
}

struct experiment* experiment_init(storage_t storage) {
    struct experiment* exp = malloc(sizeof(struct experiment));
    if (exp == NULL) {
        return NULL;
    }

    sys_slist_init(&exp->columns);
    exp->column_count = 0;
    exp->columns_flushed = false;

    sys_slist_init(&exp->rows);
    exp->rows_count = 0;

    exp->storage = storage;

    return exp;
}

void experiment_free(struct experiment * exp) {
    if (exp == NULL) {
        return;
    }

    experiment_flush(exp);
    free_columns(exp);

    free(exp);
}

int experiment_add_column(
    struct experiment *experiment,
    const char * name,
    const char * units
) {
    struct experiment_caption* node = malloc(sizeof(struct experiment_caption));

    const size_t name_len = strlen(name);
    node->column_name = malloc(name_len + 1);
    if (node->column_name == NULL) {
        return -ENOMEM;
    }
    strncpy(node->column_name, name, name_len + 1);

    const size_t unit_len = strlen(units);
    node->unit = malloc(unit_len + 1);
    if (node->unit == NULL) {
        return -ENOMEM;
    }
    strncpy(node->unit, units, unit_len + 1);

    sys_slist_append(&experiment->columns, &node->node);
    experiment->column_count++;

    return 0;
}

struct experiment_row* experiment_row_new(void) {
    // Allocate values for the row.
    return malloc(sizeof(struct experiment_row));
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
        const char* const row_str = format_row(row_str_buf, entry);

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
