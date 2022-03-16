#include <stddef.h>

#include "utils.h"

thread_t * pop_first(thread_t ** fifo){
    thread_t * first = *fifo;
    first->next = NULL;
    *fifo = (*fifo)->next;
    return first;
}

void push_last(thread_t * fifo, thread_t * last){
    while(fifo->next){
        fifo = fifo->next;
    }
    fifo->next = last;
}

// ! les traitements autres que sur la fifo ne sont pas supportés
void run_next_thread(thread_t ** fifo, thread_t ** t_running){
    // push_last(*fifo, *t_running);
    *t_running = pop_first(fifo);
}