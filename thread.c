#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <valgrind/valgrind.h>

#include "thread.h"
#include "utils.h"

//#define DEBUG_PRINT(...) printf(__VA_ARGS__)
#define DEBUG_PRINT(...)

TAILQ_HEAD(, thread_struct) runq;
TAILQ_HEAD(, thread_struct) freeq;
thread_t _running_thread = NULL;
thread_t _main_thread = NULL;

void set_running_thread(thread_t thread)
{
	//DEBUG_PRINT("set_running_thread: %p\n", thread);
	thread->status = RUNNING;
	_running_thread = thread;
}

void* thread_handler(void* (* func)(void*), void* funcarg)
{
	thread_exit(func(funcarg));
	return NULL;
}

void initialize_context(thread_t thread, int initialize_stack)
{
	ucontext_t* uc = malloc(sizeof(ucontext_t));
	getcontext(uc);
	if (initialize_stack)
	{
		uc->uc_stack.ss_size = THREAD_STACK_SIZE;
		uc->uc_stack.ss_sp = malloc(THREAD_STACK_SIZE);
		thread->valgrind_stackid = VALGRIND_STACK_REGISTER(uc->uc_stack.ss_sp,
				uc->uc_stack.ss_sp + uc->uc_stack.ss_size);
	}
	else
	{
		thread->valgrind_stackid = -1;
	}

	thread->context = uc;

}

void initialize_thread(thread_t thread, int is_main_thread)
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
		DEBUG_PRINT("Freeing thread %p\n", thread_to_free);
		VALGRIND_STACK_DEREGISTER(thread_to_free->valgrind_stackid);
		free(thread_to_free->context->uc_stack.ss_sp);
		free(thread_to_free->context);
		free(thread_to_free);
	}

}

__attribute__((constructor)) void initialize_runq()
{
	TAILQ_INIT(&runq);
	TAILQ_INIT(&freeq);

	thread_t main_thread = malloc(sizeof(struct thread_struct));
	initialize_thread(main_thread, 1);
	set_running_thread(main_thread);
	_running_thread = _main_thread = main_thread;
	DEBUG_PRINT("Main thread created %p\n", _main_thread);
}

void print_queue()
{
	DEBUG_PRINT("MAIN: %p\n", _main_thread);
	/*thread_t thread;
	TAILQ_FOREACH(thread, &runq, field) DEBUG_PRINT("%s%p%s -> ",
			thread == _running_thread ? "[" : "", thread, thread == _running_thread ? "]" : "");*/
	thread_t n1 = TAILQ_FIRST(&runq), n2;
	while (n1 != NULL)
	{
		n2 = TAILQ_NEXT(n1, next_runq);
		DEBUG_PRINT("%s%p%s -> ", n1 == _running_thread ? "[" : "", n1, n1 == _running_thread ? "]" : "");
		n1 = n2;
	}
	DEBUG_PRINT("\n");
}

__attribute__((destructor)) void destroy_runq()
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
	//DEBUG_PRINT("Yielding from thread %p\n", _running_thread);
	thread_t yielding_thread = _running_thread;
	thread_t next_thread = TAILQ_FIRST(&runq);

	if (next_thread == NULL)
	{
		DEBUG_PRINT("No thread to yield to\n");
		return 0;
	}

	//DEBUG_PRINT("Thread %p (%d) yielding to thread %p (%d)\n", yielding_thread, yielding_thread->status, next_thread, next_thread->status);
	yielding_thread->status = READY;
	TAILQ_INSERT_TAIL(&runq, yielding_thread, next_runq);
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
	set_running_thread(to_wait);
	TAILQ_REMOVE(&runq, to_wait, next_runq);

	swapcontext(waiting_thread->context, to_wait->context);

	if (to_wait->status != TERMINATED) // for 33-switch-many-cascade
	{
		TAILQ_INSERT_TAIL(&freeq, to_wait, next_freeq);
		to_wait->is_in_freeq = 1;
		// If the thread has not terminated, it means that it has been interrupted
		while (to_wait->status != TERMINATED)
		{
			thread_yield();
		}
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
		self->status = WAITING;
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
