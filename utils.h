#ifndef __UTILS_H__
#define __UTILS_H__

#include "thread.h"

#define NO_LOCK -1
#define NO_OWNER NULL
#define BEGIN_SIZE 2

typedef struct thread_mutex{
    thread_t owner; // NULL if no owner
} thread_mutex_t;

struct mutex_chain{
    int mutex_index;
    struct mutex_chain * next;
};

// Resize the allocated array of thread_mutex_t pointers (return 0 if request failed)
int realloc_mutex_table(thread_mutex_t *** table, size_t * current_size);

// Create a mutex_chain element
struct mutex_chain * alloc_mutex_element(int mutex_index, struct mutex_chain * next);

// Free the given mutex chain
void free_mutex_chain(struct mutex_chain * begin);

// Analyse if the given mutex index belongs to the mutex chain
int belong_to(int mut_ind, struct mutex_chain * begin);

// Assert if a dead lock occured
int detect_dead_lock_from_begining(thread_mutex_t ** table, int begin_lock);

// Give the first process to execute to come back on a lock chain
thread_t get_last_mutex_queue(thread_t t);

#endif // __UTILS_H__