// extern thread_t thread_self(void);
// extern int thread_create(thread_t *newthread, void *(*func)(void *), void *funcarg);
// extern int thread_yield(void);
// extern int thread_join(thread_t thread, void **retval);
// extern void thread_exit(void *retval);// __attribute__ ((__noreturn__));

// int thread_mutex_init(thread_mutex_t *mutex);
// int thread_mutex_destroy(thread_mutex_t *mutex);
// int thread_mutex_lock(thread_mutex_t *mutex);
// int thread_mutex_unlock(thread_mutex_t *mutex);

// int get_mutex_ind();
// void realloc_mutex_table(thread_mutex_t *** table, size_t * current_size);

// void set_running_thread(thread_t thread)
// void* thread_handler(void* (* func)(void*), void* funcarg)
// void initialize_context(thread_t thread, int initialize_stack)
// void initialize_thread(thread_t thread, int is_main_thread)
// void free_thread(thread_t thread_to_free)
// __attribute__((unused)) __attribute__((constructor)) void initialize_runq()
// void print_queue() // NOT TO TEST
// __attribute__((unused)) __attribute__((destructor)) void destroy_runq()
// thread_t get_next_thread()
// void set_next_thread(thread_t next_thread)



// perf
// cas limites
