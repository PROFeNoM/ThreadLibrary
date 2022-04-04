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
void realloc_mutex_table(thread_mutex_t *** table, size_t * current_size) {
    if((*table) == NULL){
        *current_size = BEGIN_SIZE;
        *table = malloc(sizeof(void *) * (*current_size));
        if((*table) == NULL){
        printf("Error on mutex table allocation");
        exit(0); // error
        }
    }
    size_t old_size = *current_size;
    *current_size = (*current_size) * 2;
    *table = realloc(*table, sizeof(void *) * (*current_size));
    if((*table) == NULL){
        printf("Error on mutex table allocation");
        exit(0); // error
    }
    for(size_t i = old_size; i < (*current_size); ++i){
        (*table)[i] = NULL;
    }
}