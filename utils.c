#include <stddef.h>
#include <stdio.h>

#include "utils.h"

thread_t pop_first(thread_t * fifo){
    thread_t first = *fifo;
    if (*fifo)
        *fifo = (*fifo)->next;
    return first;
}

void push_last(thread_t * fifo, thread_t last){
    if (*fifo == NULL) {
        *fifo = last;
        return;
    }
    thread_t ptr = *fifo;
    while (ptr->next){
        ptr = ptr->next;
    }
    ptr->next = last;
}

// ! les traitements autres que sur la fifo ne sont pas supportés
void run_next_thread(thread_t * fifo, thread_t * t_running){
    push_last(fifo, *t_running);
    *t_running = pop_first(fifo);
}