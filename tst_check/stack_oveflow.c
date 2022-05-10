#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "thread.h"

struct heavy
{
    long long int Alexandre;
    long long int Louis;
    long long int Mathilde;
    long long int Enzo;
    long long int Sylvain;
};

void * lets_go(void *)
{
    struct heavy heavy_struct;
    lets_go(&heavy_struct);
    return NULL;
}

int main(int argc, char ** argv) // Executer la fonction récursive dans un thread particulier
{
    thread_t t;
    void * res;
    int err = thread_create(&t, lets_go, NULL);
    assert(!err);
    err = thread_join(t, &res);
    assert(!err);

    return 0;
}