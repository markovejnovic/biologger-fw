typedef struct storage* storage_t;
typedef struct experiment* stored_experiment_t;

storage_t storage_init(void);
int storage_close(storage_t);

stored_experiment_t storage_stored_experiment_new(storage_t);
int storage_stored_experiment_close(stored_experiment_t);
