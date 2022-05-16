#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/time.h>
#include "../thread.h"
#include <pthread.h>
#include <time.h>

#define MAX 524288
#define THREAD_MAX 524288


// array of size MAX
int a[MAX];
int part = 0;

// merge function for merging two parts
void merge(int low, int mid, int high)
{
    //int* left = new int[mid - low + 1];
    //int* right = new int[high - mid];
    int* left = calloc(mid - low + 1, sizeof(int));
    int* right = calloc(high - mid, sizeof(int));

    // n1 is size of left part and n2 is size
    // of right part
    int n1 = mid - low + 1, n2 = high - mid, i, j;

    // storing values in left part
    for (i = 0; i < n1; i++)
        left[i] = a[i + low];

    // storing values in right part
    for (i = 0; i < n2; i++)
        right[i] = a[i + mid + 1];

    int k = low;
    i = j = 0;

    // merge left and right in ascending order
    while (i < n1 && j < n2) {
        if (left[i] <= right[j])
            a[k++] = left[i++];
        else
            a[k++] = right[j++];
    }

    // insert remaining values from left
    while (i < n1) {
        a[k++] = left[i++];
    }

    // insert remaining values from right
    while (j < n2) {
        a[k++] = right[j++];
    }

    free(left);
    free(right);
}

// merge sort function
void merge_sort(int low, int high)
{
    // calculating mid point of array
    int mid = low + (high - low) / 2;
    if (low < high) {

        // calling first half
        merge_sort(low, mid);

        // calling second half
        merge_sort(mid + 1, high);

        // merging the two halves
        merge(low, mid, high);
    }
}

// thread function for multi-threading
void* merge_sort_threads(void* arg)
{
    // which part out of 4 parts
    int thread_part = part++;

    // calculating low and high
    int low = thread_part * (MAX / THREAD_MAX);
    int high = (thread_part + 1) * (MAX / THREAD_MAX) - 1;

    // evaluating mid point
    int mid = low + (high - low) / 2;
    if (low < high) {
        merge_sort(low, mid);
        merge_sort(mid + 1, high);
        merge(low, mid, high);
    }
}

int main()
{

    for (int i = 0; i < MAX; i++)
        a[i] = rand() % 16384;


    struct timeval tv1, tv2;
    unsigned long us;

    thread_t threads[THREAD_MAX];
    gettimeofday(&tv1, NULL);

    for (int i = 0; i < THREAD_MAX; i++)
    {
        thread_create(&threads[i], merge_sort_threads, (void*)NULL);
        /*pthread_create(&threads[i], NULL, merge_sort_threads,
                                        (void*)NULL);*/
    }


    for (int i = 0; i < THREAD_MAX; i++)
    {
        thread_join(threads[i], NULL);
    }

    // Merge final parts
    int left = 0;
    int right = 0;
    int ecart = 0;
    int mid = 0;
    int nb_threads = THREAD_MAX;
    int thread = 0;

    for(int i=0; i<THREAD_MAX-1; i++)
    {

        ecart = MAX * 2 / nb_threads;

        left = thread * ecart;
        right = (thread+1) * ecart - 1;
        mid = (left + right) / 2;

        if (right == MAX - 1)
        {
            thread = 0;
            nb_threads /= 2;
        }
        else
        {
            thread++;
        }

        merge(left, mid, right);
    }

    gettimeofday(&tv2, NULL);
    us = (tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec);

    printf("TIME: %li\n", us);

   for(int i=0; i<MAX; i++)
   {
        if (i < MAX-1 && a[i] > a[i+1])
            printf("ERREUR");

       //printf("value: %d\n", a[i]);
   }

    return 0;
}
