#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <valgrind/valgrind.h>
#include <sys/mman.h>
#include <unistd.h>
#include <signal.h>
#include <strings.h>

#include "thread.h"

#define SEGV_STACK_SIZE MINSIGSTKSZ + 4096

//#define DEBUG_PRINT(...) printf(__VA_ARGS__)
#define DEBUG_PRINT(...)

#define OPT_PROTECT_STACK 0
#define OPT_DEADLOCK 1

static inline void sigsegv_handler(int signum, siginfo_t *info, void *data) {
    printf("Received signal\n");
    exit(1);
}

TAILQ_HEAD(, thread_struct) runq;
TAILQ_HEAD(, thread_struct) freeq;
thread_t _running_thread = NULL;
thread_t _main_thread = NULL;

static inline void set_running_thread(thread_t thread)
{
	DEBUG_PRINT("set_running_thread: %p\n", thread);
	thread->status = RUNNING;
	_running_thread = thread;
}

void* thread_handler(void* (* func)(void*), void* funcarg)
{
	thread_exit(func(funcarg));
	return NULL;
}

static inline void initialize_context(thread_t thread, int initialize_stack)
{
	ucontext_t* uc = malloc(sizeof(ucontext_t));
	getcontext(uc);
	if (initialize_stack)
	{
		uc->uc_stack.ss_size = THREAD_STACK_SIZE;
    if (OPT_PROTECT_STACK == 1)
    {
        uc->uc_stack.ss_sp = valloc(THREAD_STACK_SIZE);
        mprotect(uc->uc_stack.ss_sp, getpagesize(), PROT_NONE);
    }
    else
    {
        uc->uc_stack.ss_sp = malloc(THREAD_STACK_SIZE);
    }
		thread->valgrind_stackid = VALGRIND_STACK_REGISTER(uc->uc_stack.ss_sp,
				uc->uc_stack.ss_sp + uc->uc_stack.ss_size);
	}
	else
	{
		thread->valgrind_stackid = -1;
	}

	thread->context = uc;

}

static inline void initialize_thread(thread_t thread, int is_main_thread)
{
	initialize_context(thread, !is_main_thread);
	thread->status = READY;
	thread->previous_thread = NULL;
	thread->retval = NULL;
	thread->is_in_freeq = 0;
}

void free_thread(thread_t thread_to_free)
{
	if (thread_to_free && !thread_to_free->is_in_freeq && thread_to_free != _main_thread)
	{
		DEBUG_PRINT("Freeing thread %p (%d)\n", thread_to_free, thread_to_free->is_in_freeq);
		VALGRIND_STACK_DEREGISTER(thread_to_free->valgrind_stackid);
        if (OPT_PROTECT_STACK == 1)
        {
            mprotect(thread_to_free->context->uc_stack.ss_sp, getpagesize(), PROT_READ | PROT_WRITE);
        }
		free(thread_to_free->context->uc_stack.ss_sp);
		free(thread_to_free->context);
		free(thread_to_free);
	}

}

__attribute__((constructor)) void initialize_lib()
{
    if (OPT_PROTECT_STACK == 1)
    {
        struct sigaction action;
        bzero(&action, sizeof(action));
        action.sa_flags = SA_SIGINFO|SA_STACK;
        action.sa_sigaction = &sigsegv_handler;
        sigaction(SIGSEGV, &action, NULL);

        stack_t segv_stack;
        segv_stack.ss_sp = valloc(SEGV_STACK_SIZE);
        segv_stack.ss_flags = 0;
        segv_stack.ss_size = SEGV_STACK_SIZE;
        sigaltstack(&segv_stack, NULL);
    }

	TAILQ_INIT(&runq);
	TAILQ_INIT(&freeq);

	thread_t main_thread = malloc(sizeof(struct thread_struct));
	initialize_thread(main_thread, 1);
	set_running_thread(main_thread);
	_running_thread = _main_thread = main_thread;
	DEBUG_PRINT("Main thread created %p\n", _main_thread);
}

__attribute__((destructor)) void destroy_lib()
{
	thread_t n1 = TAILQ_FIRST(&runq), n2;
	while (n1 != NULL)
	{
		n2 = TAILQ_NEXT(n1, next_runq);
		free_thread(n1);
		n1 = n2;
	}

	n1 = TAILQ_FIRST(&freeq);
	while (n1 != NULL)
	{
		n2 = TAILQ_NEXT(n1, next_freeq);
		if (n1 != _main_thread)
		{
			DEBUG_PRINT("Freeing thread freeq %p\n", n1);
			VALGRIND_STACK_DEREGISTER(n1->valgrind_stackid);
			free(n1->context->uc_stack.ss_sp);
			free(n1->context);
			free(n1);
		}
		n1 = n2;
	}

	if (_main_thread != _running_thread) free_thread(_running_thread);
	else
		DEBUG_PRINT("Main thread %p is running\n", _main_thread);
	free(_main_thread->context);
	free(_main_thread);

}

/* recuperer l'identifiant du thread courant.
 */
thread_t thread_self(void)
{
	return _running_thread;
}

/* creer un nouveau thread qui va exécuter la fonction func avec l'argument funcarg.
 * renvoie 0 en cas de succès, -1 en cas d'erreur.
 */
int thread_create(thread_t* newthread, void* (* func)(void*), void* funcarg)
{
	// Create a new thread
	(*newthread) = malloc(sizeof(struct thread_struct));
	DEBUG_PRINT("Creating thread %p\n", *newthread);
	initialize_thread(*newthread, 0);
	makecontext((*newthread)->context, (void (*)(void))thread_handler, 2, func, funcarg);

	// Add thread at the end of the list
	TAILQ_INSERT_TAIL(&runq, *newthread, next_runq);
	return 0;
}

/* passer la main à un autre thread.
 */
int thread_yield(void)
{
	DEBUG_PRINT("Yielding from thread %p\n", _running_thread);
	thread_t yielding_thread = _running_thread;
	thread_t next_thread = TAILQ_FIRST(&runq);

	if (next_thread == NULL || yielding_thread->previous_thread)
	{
		DEBUG_PRINT("No thread to yield to\n");
		return 0;
	}

	DEBUG_PRINT("Thread %p (%d) yielding to thread %p (%d)\n", yielding_thread, yielding_thread->status, next_thread, next_thread->status);
	if (yielding_thread->status == RUNNING)
	{
		yielding_thread->status = READY;
		TAILQ_INSERT_TAIL(&runq, yielding_thread, next_runq);
		DEBUG_PRINT("Adding %p to the runq\n", yielding_thread);
	}
	DEBUG_PRINT("Removed next_thread %p (%d) from runq\n", next_thread, next_thread->status);
	set_running_thread(next_thread);
	TAILQ_REMOVE(&runq, next_thread, next_runq);

	return swapcontext(yielding_thread->context, next_thread->context);
}

/* attendre la fin d'exécution d'un thread.
 * la valeur renvoyée par le thread est placée dans *retval.
 * si retval est NULL, la valeur de retour est ignorée.
 */
int thread_join(thread_t thread, void** retval)
{
	DEBUG_PRINT("Joining thread %p\n", thread);
	thread_t waiting_thread = _running_thread;

	thread_t to_wait = thread;

	if (to_wait->previous_thread != NULL && to_wait->previous_thread != waiting_thread)
	{
		return -1;
	}

    if (OPT_DEADLOCK) {
        thread_t t = waiting_thread->previous_thread;
        while (t != NULL) {
            if (t == waiting_thread) {
                return -1;
            }
            t = t->previous_thread;
        }
    }

	if (to_wait->status == TERMINATED)
	{
		if (retval != NULL)
			*retval = to_wait->retval;
		DEBUG_PRINT("This thread has already existed: freeing thread %p\n", to_wait);
		free_thread(to_wait);
		return 0;
	}

	to_wait->previous_thread = waiting_thread;
	waiting_thread->status = WAITING;
	DEBUG_PRINT("Thread %p (%d) waiting for thread %p (%d)\n", waiting_thread, waiting_thread->status, to_wait, to_wait->status);
	if (to_wait->status != WAITING)
	{
		set_running_thread(to_wait);
		TAILQ_REMOVE(&runq, to_wait, next_runq);
		swapcontext(waiting_thread->context, to_wait->context);
	}
	else
	{
		DEBUG_PRINT("Thread %p (%d) has not terminated\n", to_wait, to_wait->status);
		TAILQ_INSERT_TAIL(&freeq, to_wait, next_freeq);
		to_wait->is_in_freeq = 1;
		while (to_wait->status != TERMINATED)
		{
			waiting_thread->status = WAITING;
			thread_yield();
		}
		DEBUG_PRINT("##### Thread %p (%d) has terminated\n", to_wait, to_wait->status);

		if (retval != NULL)
			*retval = to_wait->retval;

		return 0;
	}


	if (retval != NULL)
		*retval = to_wait->retval;

	DEBUG_PRINT("Thread %p (%d) has been joined, back to %p (%d)\n", to_wait, to_wait->status, _running_thread, _running_thread->status);
	DEBUG_PRINT("->Freeing thread %p\n", to_wait);
	free_thread(to_wait);

	return 0;
}

/* terminer le thread courant en renvoyant la valeur de retour retval.
 * cette fonction ne retourne jamais.
 *
 * L'attribut noreturn aide le compilateur à optimiser le code de
 * l'application (élimination de code mort). Attention à ne pas mettre
 * cet attribut dans votre interface tant que votre thread_exit()
 * n'est pas correctement implémenté (il ne doit jamais retourner).
 */
void thread_exit(void* retval)
{
	DEBUG_PRINT("Exiting thread %p\n", _running_thread);
	thread_t exiting_thread = _running_thread;

	exiting_thread->retval = retval;
	exiting_thread->status = TERMINATED;

	if (exiting_thread->previous_thread != NULL)
	{
		DEBUG_PRINT("Thread %p is waiting for thread %p\n", exiting_thread->previous_thread, exiting_thread);
		set_running_thread(exiting_thread->previous_thread);
		if (exiting_thread == _main_thread) swapcontext(exiting_thread->context, exiting_thread->previous_thread->context);
		else setcontext(exiting_thread->previous_thread->context);

	}
	else
	{
		DEBUG_PRINT("Nobody is waiting for thread %p\n", exiting_thread);
		thread_t next_thread = TAILQ_FIRST(&runq);
		if (next_thread != NULL)
		{
			DEBUG_PRINT("Thread %p is the next thread to run\n", next_thread);
			set_running_thread(next_thread);
			TAILQ_REMOVE(&runq, next_thread, next_runq);
			if (exiting_thread == _main_thread) swapcontext(exiting_thread->context, next_thread->context);
			else setcontext(next_thread->context);
		}
		else
		{
			DEBUG_PRINT("No thread to run\n");
			if (exiting_thread == _main_thread)
			{
				DEBUG_PRINT("Main thread exiting\n");
				exit(0);
			}
			else
			{
				DEBUG_PRINT("Fallback to main thread\n");
				//set_running_thread(_main_thread);
				setcontext(_main_thread->context);
			}
		}
	}
}

// Here just so 61-mutex.c can compile, although it obviously doesn't yield the correct result
int thread_mutex_init(thread_mutex_t* mutex)
{
	mutex->owner = NULL;
	mutex->is_valid = 1;
	TAILQ_INIT(&mutex->waiting_threads);
	return 0;
}

int thread_mutex_destroy(thread_mutex_t* mutex)
{
	mutex->owner = NULL;
	mutex->is_valid = 0;
	return 0;
}

int thread_mutex_lock(thread_mutex_t* mutex)
{
	thread_t self = _running_thread;

	if (mutex->owner != NULL && mutex->owner != self)
	{
		DEBUG_PRINT("Thread %p is waiting for mutex %p (%p -> %d)\n", self, mutex, mutex->owner, mutex->owner->status);
		self->status = LOCKED;
		TAILQ_INSERT_TAIL(&mutex->waiting_threads, self, next_runq);
		if (mutex->owner->status == READY) TAILQ_REMOVE(&runq, mutex->owner, next_runq);
		set_running_thread(mutex->owner);
		swapcontext(self->context, mutex->owner->context);
	}

	mutex->owner = self;
	return 0;
}

int thread_mutex_unlock(thread_mutex_t* mutex)
{
	mutex->owner = NULL;

	//thread_t self = _running_thread;

	//DEBUG_PRINT("Status of thread %p is %d\n", self, self->status);
	return 0;
}
