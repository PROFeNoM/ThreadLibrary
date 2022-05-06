#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/time.h>
#include "../thread.h"

#define LOOP 10
#define LENGTH 10

/*
  This test consists in a sorting merge of a large array.
*/
int * sort_two(int * array)
{
    int length = array[0];

    if (length%2 == 1)
    {
        printf("value: %d\n", array[1]);
        return array;
    }
    else
    {
        if (array[1] > array[2])
        {
            int higher = array[1];
            array[1] = array[2];
            array[2] = higher;
        }
    }

    printf("Array: %d %d\n", array[1], array[2]);

    return array;
}

void * divide_or_not(void * a) {
    int * array = (int *) a;
    int length_array = array[0];
    printf("Length array: %d\n", length_array);

    if (length_array > 2)
    {
        int err;
        thread_t tid[2];

        int odd_number = 0;
        if (length_array % 2 == 1)
            odd_number = 1;


        int new_length_values = length_array / 2;
        int new_length_size = new_length_values + 1;

        int array_1_with_length[new_length_size];
        int array_2_with_length[new_length_size + odd_number];
        void * res_1 = malloc(sizeof(int) * new_length_size);
        void * res_2 = malloc(sizeof(int) * new_length_size);
        int i, k = 1;

        // First array
        for (i = 1; i < new_length_size; ++i) {
            array_1_with_length[i] = array[i];
        }

        // Second array
        k = i;
        for (int j = 1; j < new_length_size + odd_number; ++j) {
            array_2_with_length[j] = array[k];
            ++k;
        }

        array_1_with_length[0] = new_length_values;
        array_2_with_length[0] = new_length_values + odd_number;

        err = thread_create(&tid[0], divide_or_not, array_1_with_length);
        assert(!err);
        err = thread_create(&tid[1], divide_or_not, array_2_with_length);
        assert(!err);

        err = thread_join(tid[0], &res_1);
        assert(!err);
        err = thread_join(tid[1], &res_2);
        assert(!err);
    }

    return sort_two(array);
}


void triFusion(int i, int j, int tab[], int tmp[]) {
    if(j <= i){ return;}

    int m = (i + j) / 2;

    triFusion(i, m, tab, tmp);
    triFusion(m + 1, j, tab, tmp);

    int pg = i;
    int pd = m + 1;
    int c;


    for(c = i; c <= j; c++)
    {
            // Array empty
        if(pg == m + 1) {
            tmp[c] = tab[pd];
            pd++;
        }
        else if (pd == j + 1) {
            tmp[c] = tab[pg];
            pg++;
        }
        // Getting minimum of the arrays
        else if (tab[pg] < tab[pd])
        {
            tmp[c] = tab[pg];
            pg++;
        }
        else
        {
            tmp[c] = tab[pd];
            pd++;
        }
    }

    for(c = i; c <= j; c++) {  //copier les éléments de tmp[] à tab[]
        tab[c] = tmp[c];
    }
}


int main() {

    int array[] = {4, 3, 1, 2};
    int tmp[1000000];
    int length = sizeof(array) / sizeof(int);
    printf("Length array: %d\n", length);

    triFusion(0, length, array, tmp);

    for(int i=1; i<length+1; i++)
    {
        printf("value:%d\n", array[i]);
    }

    return 0;
}
