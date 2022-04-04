#ifndef __UTILS_H__
#define __UTILS_H__

#include "thread.h"

#define NO_LOCK -1
#define NO_OWNER NULL
#define BEGIN_SIZE 2

struct mutex_chain{
    int mutex_index;
    struct mutex_chain * next;
};

// Give the next free index in the mutex table
int get_mutex_ind();

// Resize the allocated array of thread_mutex_t pointers (return 0 if request failed)
void realloc_mutex_table(thread_mutex_t *** table, size_t * current_size);

#endif // __UTILS_H__