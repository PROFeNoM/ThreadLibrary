#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <valgrind/valgrind.h>

#include "thread.h"
#include "utils.h"

#ifdef DEBUG
#define DEBUG_PRINT(...) printf(__VA_ARGS__)
#else
#define DEBUG_PRINT(...)
#endif

TAILQ_HEAD(, thread_struct) runq;
TAILQ_HEAD(, thread_struct) lockq;
thread_t T_RUNNING = NULL;
thread_t T_MAIN = NULL;

void set_running_thread(thread_t thread)
{
	thread->status = RUNNING;
	T_RUNNING = thread;
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
}

void free_thread(thread_t thread_to_free)
{
	if (thread_to_free != NULL && thread_to_free != T_MAIN)
	{
        //printf("Freeing thread %p\n", thread_to_free);
		VALGRIND_STACK_DEREGISTER(thread_to_free->valgrind_stackid);
		free(thread_to_free->context->uc_stack.ss_sp);
		free(thread_to_free->context);
		free(thread_to_free);
		thread_to_free = NULL;
	}
}

__attribute__((constructor)) void initialize_runq()
{
	TAILQ_INIT(&runq);
	TAILQ_INIT(&lockq);

	thread_t main_thread = malloc(sizeof(struct thread_struct));
	initialize_thread(main_thread, 1);
	set_running_thread(main_thread);
	T_RUNNING = T_MAIN = main_thread;
}

void print_queue()
{
	printf("MAIN: %p\n", T_MAIN);
	/*thread_t thread;
	TAILQ_FOREACH(thread, &runq, field) printf("%s%p%s -> ",
			thread == T_RUNNING ? "[" : "", thread, thread == T_RUNNING ? "]" : "");*/
	thread_t n1 = TAILQ_FIRST(&runq), n2;
	while (n1 != NULL)
	{
		n2 = TAILQ_NEXT(n1, next_runq);
		printf("%s%p%s -> ", n1 == T_RUNNING ? "[" : "", n1, n1 == T_RUNNING ? "]" : "");
		n1 = n2;
	}
	printf("\n");
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
	if (T_MAIN != T_RUNNING) free_thread(T_RUNNING);
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
thread_t get_next_thread()
{
    thread_t current_thread = thread_self();
    if (current_thread->previous_thread != NULL)
    {
        // Run waiting thread next
		thread_t previous_thread = current_thread->previous_thread;
        return previous_thread;
    }

    if (TAILQ_EMPTY(&runq)) return T_MAIN;

    thread_t next_thread = TAILQ_FIRST(&runq);

    TAILQ_REMOVE(&runq, next_thread, next_runq);
    return next_thread;
}

void set_next_thread(thread_t next_thread)
{
    thread_t current_thread = thread_self();

    if (current_thread != next_thread)
    {
        //printf("Setting next theead to %p\n", next_thread);
        TAILQ_REMOVE(&runq, next_thread, next_runq);
        if (current_thread->status != TERMINATED && current_thread->status != LOCKED)
        {
            current_thread->status = READY;
            TAILQ_INSERT_TAIL(&runq, current_thread, next_runq);
        }
        set_running_thread(next_thread);
        swapcontext(current_thread->context, next_thread->context);
    }
}
/* creer un nouveau thread qui va exécuter la fonction func avec l'argument funcarg.
 * renvoie 0 en cas de succès, -1 en cas d'erreur.
 */
int thread_create(thread_t* newthread, void* (* func)(void*), void* funcarg)
{
	// Create a new thread
	(*newthread) = malloc(sizeof(struct thread_struct));
    //printf("Creating thread %p\n", *newthread);
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
	// Get and Swap both threads
	set_next_thread(get_next_thread());

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

    // TODO: Add some sort of compilation rule
    /*
    // Check if there would be a deadlock
    if (!to_wait || waiting_thread == to_wait || to_wait->status == WAITING) return -1;
    thread_t t1 = waiting_thread, t2;
    while (t1 != NULL)
    {
        t2 = t1->previous_thread;
        if (t2 == to_wait) return -1;
        t1 = t2;
    }
     */

	if (to_wait->status != TERMINATED)
	{
		waiting_thread->status = WAITING;
		// Current thread waits for thread to wait to terminate
		while (to_wait->status != TERMINATED)
		{
			to_wait->previous_thread = waiting_thread;
			set_next_thread(to_wait);
		}
	}

	// save retval's addr
	if (retval != NULL) *retval = to_wait->retval;

	// thread is done, free and leave
	if (to_wait != T_MAIN) free_thread(to_wait);

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
	// current thread to terminate
	thread_t current_thread = thread_self();
	current_thread->retval = retval;
	current_thread->status = TERMINATED;

	if (TAILQ_EMPTY(&runq))
	{
		if (T_MAIN == T_RUNNING) return;
		else {
			T_MAIN->status = RUNNING;
			setcontext(T_MAIN->context);
		}
	}
	else
		thread_yield();

	exit(0);
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
	if (mutex->is_valid == 0) return -1;

	thread_t current_thread = thread_self();
	if (mutex->owner == current_thread) return 0;


    // The current thread is not locked, so it can lock it
	if (mutex->owner == NULL)
	{
		mutex->owner = current_thread;
		return 0;
	}

    // If the mutex is locked, the current thread goes to sleep
	current_thread->status = LOCKED;
    TAILQ_INSERT_TAIL(&mutex->waiting_threads, current_thread, next_mutexq);

    // Current thread waits for mutex to be unlocked
	set_next_thread(mutex->owner);

	return 0;
}

int thread_mutex_unlock(thread_mutex_t* mutex)
{
	mutex->owner = NULL;

    // Wake up the thread that is waiting for the mutex
    if (!TAILQ_EMPTY(&mutex->waiting_threads))
    {
        thread_t waiting_thread = TAILQ_FIRST(&mutex->waiting_threads);
        waiting_thread->status = RUNNING;
        TAILQ_REMOVE(&mutex->waiting_threads, waiting_thread, next_mutexq);
    }

    return 0;
}
