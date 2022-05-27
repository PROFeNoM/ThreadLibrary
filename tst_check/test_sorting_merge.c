#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/time.h>
#include "../thread.h"
#include <pthread.h>
#include <string.h>
#include <time.h>

#define MAX 20000


int* main_array;


typedef struct
{
    int start;
    int end;

} arrayFusion;


arrayFusion* fusion(arrayFusion* p_array1, arrayFusion* p_array2)
{
    int size1 = p_array1->end - p_array1->start + 1;
    int size2 = p_array2->end - p_array2->start + 1;
    int size_combined = size1 + size2;

    int temp[size_combined];

    int p1 = p_array1->start;
    int p2 = p_array2->start;

    int main_p = (p1 < p2) ? p1 : p2;
    int i = 0;


    while (i < size_combined)
    {
        if (size1 <= 0)
        {
            temp[i] = main_array[p2];
            size2--;
            p2++;
        }
        else if (size2 <= 0)
        {
            temp[i] = main_array[p1];
            p1++;
            size1--;
        }
        else if (main_array[p1] < main_array[p2])
        {
            temp[i] = main_array[p1];
            size1--;
            p1++;
        }
        else
        {
            temp[i] = main_array[p2];
            size2--;
            p2++;
        }

        i++;
    }

    free(p_array1);
    free(p_array2);

    // Changing main
    for(int i=0; i<size_combined; i++)
    {
        main_array[main_p + i] = temp[i];
    }

    arrayFusion* fusion = malloc(sizeof(arrayFusion));
    fusion->start = main_p;
    fusion->end = main_p + size_combined - 1;

    return fusion;
}

void* triFusion(void* array)
{
    arrayFusion* p_array = (arrayFusion*) array;
    int size = (p_array->end - p_array->start) + 1;

    if (size <= 1)
    {
        void* p_arr = malloc(sizeof(void*));
        memcpy(p_arr, array, sizeof(void*));
        return p_arr;
    }
    else
    {
        // Dividing array by 2
        int new_start = p_array->start;
        int new_end = p_array->end;
        int new_size = size / 2;

        arrayFusion array1 = {.start = new_start, .end = new_start + new_size - 1};
        arrayFusion* parray1 = &array1;
        arrayFusion array2 = {.start = new_start + new_size, .end = new_end};
        arrayFusion* parray2 = &array2;

        // Threads part
        thread_t new1;
        thread_create(&new1, triFusion, (void*) parray1);
        thread_t new2;
        thread_create(&new2, triFusion, (void*) parray2);

        arrayFusion sorted_arr1;
        arrayFusion sorted_arr2;
        void* test1 = (void *) &sorted_arr1;
        void* test2 = (void *) &sorted_arr2;
        void** retval1 = &test1;
        void** retval2 = &test2;
        thread_join(new1, retval1);
        thread_join(new2, retval2);

        void* p_retval1 = *retval1;
        void* p_retval2 = *retval2;
        arrayFusion* arr_retval1 = (arrayFusion*) p_retval1;
        arrayFusion* arr_retval2 = (arrayFusion*) p_retval2;


        return (void *) fusion(arr_retval1, arr_retval2);
    }
}


int main(int argc, char** argv)
{
    if (argc < 1)
    {
        printf("Besoin d'un argument passé en paramètre\n");
        exit(0);
    }

    int size_array = atoi(argv[1]);
    main_array = (int*) malloc(size_array * sizeof(int));

    for(int i=0; i<size_array; i++)
    {
        main_array[i] = rand() % MAX;
    }

    struct timeval tv1, tv2;
    unsigned long us;

    gettimeofday(&tv1, NULL);

    arrayFusion arr = {.start = 0, .end = size_array-1};
    arrayFusion* parr = &arr;
    parr->start = 0;
    parr->end = size_array-1;
    thread_t main_thread;
    thread_create(&main_thread, triFusion, (void *) parr);
    void* retval_main = NULL;
    thread_join(main_thread, &retval_main);

    gettimeofday(&tv2, NULL);
    us = (tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec);

    printf("TIME: %li\n", us);
    for(int i=0; i<size_array; i++)
    {
        printf("value: %d\n", main_array[i]);
    }

   free(main_array);
   free(retval_main);

    return 0;
}
