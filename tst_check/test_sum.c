#include <stdio.h>
#include <assert.h>
//#include <pthread.h>
#include <stdlib.h>
#include <sys/time.h>
#include "../thread.h"



typedef struct data{
    int* arr;
    int thread_num;
} data;


void* return_value(void* p)
{
    data* ptr = (data*) p;
    int n = ptr->thread_num;

    int* thread_sum = (int*) calloc(1, sizeof(int));
    thread_sum[0] = ptr->arr[n];

    thread_exit(thread_sum);
}


int main()
{

    int arrSize = 10;
    int threadReturn = 1;
    int iterateur = arrSize / threadReturn;

    struct timeval tv1, tv2;
    unsigned long us;

    // Declare integer array [1,2,3,4,5,6,7,8,9,10]:
    int* int_arr = (int*) calloc(arrSize, sizeof(int));
    for(int i = 0; i < arrSize; i++)
    {
        int_arr[i] = i + 1;
    }


    thread_t tid[iterateur];
    data thread_data[iterateur];

    gettimeofday(&tv1, NULL);
    for(int i = 0; i<iterateur; i++)
    {
        thread_data[i].thread_num = i;
        thread_data[i].arr = int_arr;
        fprintf(stderr, "Creating thread n%d\n", thread_data[i].thread_num);

        thread_create(&tid[i], return_value, &thread_data[i]);
    }

    fprintf(stderr, "--- End creating thread ---\n");
    int** sum = (int**) calloc(iterateur, sizeof(int*));
    fprintf(stderr, "--- Starting joining thread ---\n");

    for(int i = 0; i<iterateur; i++)
    {
        thread_join(tid[i], (void**) &sum[i]);
        fprintf(stderr, "join thread n%d\n", i);
    }

    int result = 0;
    for(int i = 0; i<iterateur; i++)
    {
        printf("sum[i]: %d\n", *sum[i]);
        result += *sum[i];
        free(sum[i]);
    }
    gettimeofday(&tv2, NULL);
    us = (tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec);

    printf("TIME: %li\n", us);



    free(sum);
    free(int_arr);

    printf("Sum array: %d\n", result);

    return 1;
}
