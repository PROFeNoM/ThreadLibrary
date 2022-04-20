#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "../thread.h"

#define LOOP 10
#define LENGTH 10

/*
  This test consists in a sorting merge of a large array.
*/

int * sort_two(int * array) {
  if (array[0] > array[1]) {
    int higher = array[0];
    array[0] = array[1];
    array[1] = higher;
  }
  return array;
}

void * divide_or_not(void * a) {
  int * array = (int *) a;
  int length_array = array[0];
  if (length_array > 2) {
    int err;
    thread_t * newthread_1, * newthread_2;
    int new_length = length_array / 2;
    int array_1_with_length[new_length + 1];
    int array_2_with_length[new_length + 1];
    void * res_1 = malloc(sizeof(int) * new_length);
    void * res_2 = malloc(sizeof(int) * new_length);
    int i, k = 1;

    for (i = 1; i < new_length + 1; ++i) {
      array_1_with_length[i] = array[i];
    }
    k = i;
    for (int j = 1; j < new_length + 1; ++j) {
      array_2_with_length[j] = array[i];
      ++k;
    }
    array_1_with_length[0] = new_length;
    array_2_with_length[0] = new_length;

    err = thread_create(newthread_1, divide_or_not, array_1_with_length);
    assert(!err);
    err = thread_create(newthread_2, divide_or_not, array_2_with_length);
    assert(!err);

    err = thread_join(*newthread_1, &res_1);
    assert(!err);
    err = thread_join(*newthread_2, &res_2);
    assert(!err);
  }
  else {
    return sort_two(array);
  }
}

void sorting_merge(int * array) {
  int number_threads = LENGTH / 2;
  int * array_two = malloc(sizeof(int) * 2);
  for (int i = 0; i < number_threads; ++i) {
    array_two = divide_or_not(array);
    printf("0 = %d, 1 = %d\n", array_two[0], array_two[1]);
  }
}

int main() {

  int array[] = {LENGTH, 1, 3, 5, 4, 9, 6, 5, 2, 1, 8};
  sorting_merge(array);

  return 0;
}
