#ifndef __UTILS_H__
#define __UTILS_H__

#define NO_LOCK -1
#define NO_OWNER NULL

#include "thread.h"
int create_lock(void ** lock_table);

void realloc_lock_table(void *** table, int * current_size);

int is_lockable(int lock);

int detect_dead_lock();

#endif // __UTILS_H__