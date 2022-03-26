#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <valgrind/valgrind.h>
#include <sys/queue.h>

#include "thread.h"

#define THREAD_STACK_SIZE 64*1024 // CONSTANTE à requestionner §§§§§§§§§

enum STATUS
{
	JOINING, TERMINATED, RUNNING
};

struct thread_struct
{
	TAILQ_ENTRY(thread_struct) field;

	ucontext_t* context;
	thread_t previous_thread;
	int valgrind_stackid;
	enum STATUS status;

	void* retval;
};

TAILQ_HEAD(queue, thread_struct) runq;
thread_t T_RUNNING;
thread_t T_MAIN;

void* thread_handler(void *(*func)(void*), void *funcarg) {
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
	} else
	{
		thread->valgrind_stackid = -1;
	}

	thread->context = uc;

}

void initialize_thread(thread_t thread, int is_main_thread)
{
	initialize_context(thread, !is_main_thread);
	thread->status = RUNNING;
	thread->previous_thread = NULL;

}

void free_thread(thread_t thread_to_free)
{
	// NOTE: Let the destructor free the main, at the end of execution ?
	// NOTE: While testing, the main thread has never been called as a parameter of this function
	// NOTE: However, if, eventually, the main thread happens to be called by this func, then an if clause should be used not to free it
	free(thread_to_free->context->uc_stack.ss_sp);
	free(thread_to_free->context);
	free(thread_to_free);
	thread_to_free = NULL;
}

__attribute__((unused)) __attribute__((constructor)) void initialize_runq()
{
	TAILQ_INIT(&runq);

	thread_t main_thread = malloc(sizeof(struct thread_struct));
	initialize_thread(main_thread, 1);
	TAILQ_INSERT_HEAD(&runq, main_thread, field);

	T_RUNNING = T_MAIN = main_thread;
}

__attribute__((unused)) __attribute__((destructor)) void destroy_runq()
{
	if (!TAILQ_EMPTY(&runq))
	{
		thread_t thread_to_free;
		TAILQ_FOREACH(thread_to_free, &runq, field);
		if (thread_to_free) free_thread(thread_to_free);
	}

	//if (T_MAIN != T_RUNNING) free_thread(T_RUNNING);
	free(T_MAIN->context);
	free(T_MAIN);
	T_MAIN = NULL;
}


/* recuperer l'identifiant du thread courant.
 */
thread_t thread_self(void)
{
	return T_RUNNING;
}

/* creer un nouveau thread qui va exécuter la fonction func avec l'argument funcarg.
 * renvoie 0 en cas de succès, -1 en cas d'erreur.
 */
int thread_create(thread_t* newthread, void* (* func)(void*), void* funcarg)
{
	// Create a new thread
	(*newthread) = malloc(sizeof(struct thread_struct));

	initialize_thread(*newthread, 0);
	makecontext((*newthread)->context, (void (*)(void))thread_handler, 2, func, funcarg);

	// Add thread at the end of the list
	TAILQ_INSERT_TAIL(&runq, *newthread, field);

	return 0;
}

/* passer la main à un autre thread.
 */
int thread_yield(void)
{
	thread_t current_thread = thread_self();
	thread_t next_thread = TAILQ_NEXT(current_thread, field);

	// End of the list ==> go back the the beginning
	if (next_thread == NULL) next_thread = TAILQ_FIRST(&runq);

	// Search for the next not joining thread
	while (current_thread != next_thread && next_thread->status == JOINING)
	{
		next_thread = TAILQ_NEXT(next_thread, field);
		if (next_thread == NULL) next_thread = TAILQ_FIRST(&runq);
	}

	// Swap both threads
	if (next_thread != current_thread) {
		T_RUNNING = next_thread;
		swapcontext(current_thread->context, thread_self()->context);
	}

	return 0;
}

/* attendre la fin d'exécution d'un thread.
 * la valeur renvoyée par le thread est placée dans *retval.
 * si retval est NULL, la valeur de retour est ignorée.
 */
int thread_join(thread_t thread, void** retval)
{
	thread_t to_wait = thread;
	thread_t waiting_thread = thread_self();

	if (to_wait->status != TERMINATED)
	{
		waiting_thread->status = JOINING;

		// Current thread waits for thread to wait to terminate
		while (to_wait->status != TERMINATED)
		{
			T_RUNNING = to_wait;
			to_wait->previous_thread = waiting_thread;
			swapcontext(waiting_thread->context, T_RUNNING->context);
		}
	}

	// save retval's addr
	if (retval != NULL) *retval = to_wait->retval;

	// thread is done, free and leave
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
	//printf("Called thread exit on %p\n", thread_self());
	// current thread to terminate
	thread_t current_thread = thread_self();
	current_thread->retval = retval;
	current_thread->status = TERMINATED;
	TAILQ_REMOVE(&runq, current_thread, field);

	thread_t next_thread;

	// Check if a thread is waiting for the current thread to terminate (cf. thread_join)
	if (current_thread->previous_thread != NULL)
	{
		// Run waiting thread next
		next_thread = current_thread->previous_thread;
		next_thread->status = RUNNING;
	}
	else
	{
		next_thread = TAILQ_NEXT(current_thread, field);

		if (next_thread == NULL)
		{
			if (TAILQ_EMPTY(&runq))
			{
				// There's no other thread than the main one
				setcontext(T_MAIN->context);
				exit(0);
			}
			else
			{
				// Take the logical next thread
				next_thread = TAILQ_FIRST(&runq);
			}
		}
	}

	T_RUNNING = next_thread;

	// Prioritize list's threads over the main one
	if (current_thread == T_MAIN && !TAILQ_EMPTY(&runq))
		swapcontext(current_thread->context, T_RUNNING->context);
	else
		setcontext(T_RUNNING->context);

	exit(0);
}

// Here just so 61-mutex.c can compile, although it obviously doesn't yield the correct result
int thread_mutex_init(thread_mutex_t *mutex){(void)(mutex); return 0;}
int thread_mutex_destroy(thread_mutex_t *mutex){(void)(mutex); return 0;}
int thread_mutex_lock(thread_mutex_t *mutex){(void)(mutex); return 0;}
int thread_mutex_unlock(thread_mutex_t *mutex){(void)(mutex); return 0;}
