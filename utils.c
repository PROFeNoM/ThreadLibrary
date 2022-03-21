#include <stddef.h>
#include <stdio.h>

#include "utils.h"

thread_t pop_first(thread_t * fifo){
    thread_t first = *fifo;
    //printf("first : %p\n", *first);
    //(*first).next = NULL;
    printf("fifo -> %p\n", fifo);
    printf("fifo -> %p\n", *fifo);
    if (*fifo)
        *fifo = (*fifo)->next;
    else
        fifo = &(*fifo)->next;
    return first;
}

void push_last(thread_t* fifo, thread_t last){
    if (*fifo == NULL) {
        *fifo = last;
        return;
    }

    while ((*fifo)->next){
        *fifo = (*fifo)->next;
    }
    (*fifo)->next = last;
}

// ! les traitements autres que sur la fifo ne sont pas supportés
void run_next_thread(thread_t * fifo, thread_t * t_running){
    // push_last(*fifo, *t_running);
    *t_running = pop_first(fifo);
}