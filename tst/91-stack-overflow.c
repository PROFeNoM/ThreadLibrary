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

static void * thfunc(void *dummy __attribute__((unused)))
{
    struct heavy* heavy_struct;
    (void)(heavy_struct);
    return thfunc(NULL);
}

int main(int argc, char ** argv) // Executer la fonction récursive dans un thread particulier
{
    thread_t t;
    void * res;
    int err = thread_create(&t, thfunc, NULL);
    assert(!err);
    err = thread_join(t, &res);
    assert(!err);

    return EXIT_SUCCESS;
}