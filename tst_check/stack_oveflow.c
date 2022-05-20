#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "thread.h"

void handler_sig_seg(int signal){
    fprintf(stderr, "\nSIG_SEG signal received\n"
           "Ignoring the signal\n");
    // exit(1);
}

struct heavy
{
    long long int Alexandre;
    long long int Louis;
    long long int Mathilde;
    long long int Enzo;
    long long int Sylvain;
};

void * lets_go(void * p)
{
    struct heavy heavy_struct;
    lets_go(&heavy_struct);
    return NULL;
}

int main(int argc, char ** argv) // Executer la fonction récursive dans un thread particulier
{
    signal(SIGSEGV, handler_sig_seg);
    thread_t t;
    void * res;
    int err = thread_create(&t, lets_go, NULL);
    assert(!err);
    err = thread_join(t, &res);
    assert(!err);

    return 0;
}