#include "experiment.h"
#include "zephyr/sys/slist.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>

static void free_caption(struct experiment_caption* capt) {
    free(capt->column_name);
    free(capt->unit);
    free(capt);
}

static void free_row(struct experiment_row* row) {
    free(row);
}

struct experiment* experiment_init(void) {
    struct experiment* exp = malloc(sizeof(struct experiment));
    if (exp == NULL) {
        return NULL;
    }

    sys_slist_init(&exp->columns);
    exp->column_count = 0;

    sys_slist_init(&exp->rows);

    return exp;
}

void experiment_free(struct experiment * exp) {
    if (exp == NULL) {
        return;
    }

    experiment_flush(exp);

    struct experiment_caption * capt, * safe_capt;
    SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&exp->columns, capt, safe_capt, node) {
        free_caption(capt);
    }

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

struct experiment* experiment_row_new(void) {
    return malloc(sizeof(struct experiment));
}

int experiment_push_row(
    struct experiment* experiment,
    struct experiment_row* row
) {
    sys_slist_append(&experiment->rows, &row->node);

    return 0;
}

int experiment_flush(struct experiment* experiment) {
}
