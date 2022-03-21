#ifndef __UTILS_H__
#define __UTILS_H__

#include "thread.h"

thread_t pop_first(thread_t * fifo);

void push_last(thread_t* fifo, thread_t last);

// ! les traitements autres que sur la fifo ne sont pas supportés
void run_next_thread(thread_t * fifo, thread_t * t_running);

#endif // __UTILS_H__