#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "utils.h"

int get_mutex_ind() {
    static int _new_mutex_table_index = -1;
    ++_new_mutex_table_index;
    return _new_mutex_table_index;
}

// Resize the allocated array of thread_mutex_t pointers (return 0 if request failed)
int realloc_mutex_table(thread_mutex_t *** table, size_t * current_size) {
    if((*table) == NULL){
        *current_size = BEGIN_SIZE;
        *table = malloc(sizeof(void *) * (*current_size));
        return (*table) != NULL;
    }
    size_t old_size = *current_size;
    *current_size = (*current_size) * 2;
    *table = realloc(*table, *current_size);
    if((*table) == NULL){
        return 0; // error
    }
    for(int i = old_size; i < (*current_size); ++i){
        (*table)[i] = NULL;
    }
    return 1; // No error here
}

// Create a mutex_chain element
struct mutex_chain * alloc_mutex_element(int mutex_ind, struct mutex_chain * n) {
    struct mutex_chain * new = malloc(sizeof(struct mutex_chain));
    new->mutex_index = mutex_ind; new->next = n;
    return new;
}

// Free the given mutex chain
void free_mutex_chain(struct mutex_chain * begin) {
    struct mutex_chain * current = begin;
    struct mutex_chain * saved = NULL;
    while(current){
        saved = current->next;
        free(current);
        current = saved;
    }
}

// Analyse if the given mutex index belongs to the mutex chain
int belong_to(int mut_ind, struct mutex_chain * begin) {
    struct mutex_chain * current = begin;
    while(current){
        if(current->mutex_index == mut_ind){
            return 1;
        }
        current = current->next;
    }
    return 0;
}

// Assert if a dead lock occured
int detect_dead_lock_from_begining(thread_mutex_t ** table, int begin_lock) {
    thread_t t_current = NULL;
    int l_current = begin_lock;

    struct mutex_chain * chain = alloc_mutex_element(l_current, NULL); // Chain to verify an already visited lock
    
    while(1){
        if(l_current == -1){
            free_mutex_chain(chain);
            return 0; // the thread at the chain tail doesn't require a lock ==> no dead lock
        }
        t_current = table[l_current]->owner;
        if(t_current == NULL){
            free_mutex_chain(chain);
            return 0; // the lock at the chain tail doesn't have owner ==> no dead lock
        }
        l_current = t_current->waited_lock;
        if(belong_to(l_current, chain)){
            free_mutex_chain(chain);
            return 1; // the new lock belongs to the chain ==> lock cycle ==> dead lock
        }
        chain = alloc_mutex_element(l_current, chain);
    }
}

// Give the first process to execute to come back on a lock chain
thread_t get_last_mutex_queue(thread_mutex_t ** table, thread_t t) {
    thread_t t_current = t;
    int l_current = t_current->waited_lock;

    while(1){
        if((l_current == -1) || (table[l_current]->owner == NULL)){
            return t_current;
        }
        t_current = table[l_current]->owner;
        l_current = t_current->waited_lock;
    }
}