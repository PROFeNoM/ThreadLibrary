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
TAILQ_HEAD(, thread_struct) sleepq;
TAILQ_HEAD(, thread_struct) lockq;
TAILQ_HEAD(, priority_t) priorityq;
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
    thread->is_in_sleepq = 0;
	thread->is_in_lockq = 0;
	add_thread_to_default_priorityq(thread);
	thread->prioq_head = default_priority_queue;
}

void free_thread(thread_t thread_to_free)
{
	if (thread_to_free != NULL && thread_to_free != T_MAIN)
	{
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
	TAILQ_INIT(&sleepq);
	TAILQ_INIT(&lockq);
	init_priorityq();

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
	free_priorityq();
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
        previous_thread->is_in_sleepq = 0;
        TAILQ_REMOVE(&sleepq, previous_thread, next_sleepq);
        return previous_thread;
    }

    if (TAILQ_EMPTY(&runq)) return T_MAIN;

	thread_t next_thread = get_highest_priority_thread();
	/*
    thread_t next_thread = TAILQ_FIRST(&runq);

    TAILQ_REMOVE(&runq, next_thread, next_runq);
    */
    return next_thread;
}

void set_next_thread(thread_t next_thread)
{
    thread_t current_thread = thread_self();

    if (current_thread != next_thread)
    {
        //printf("Setting next theead to %p\n", next_thread);
        TAILQ_REMOVE(&runq, next_thread, next_runq);
        if (current_thread->status != TERMINATED)
        {
            current_thread->status = READY;
            TAILQ_INSERT_TAIL(&runq, current_thread, next_runq);
			//increase_thread_priority(current_thread);
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

	increase_thread_priority(waiting_thread);

    if (to_wait->is_in_sleepq) return -1;

	if (to_wait->status == WAITING || to_wait == waiting_thread) return -1;

	if (to_wait->status != TERMINATED)
	{
		waiting_thread->status = WAITING;
        waiting_thread->is_in_sleepq = 1;
		TAILQ_INSERT_TAIL(&sleepq, waiting_thread, next_sleepq);
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
	//printf("[EXIT] Removing thread %p from priority queue %d\n", current_thread, current_thread->prioq_head->priority);
	TAILQ_REMOVE(&current_thread->prioq_head->threads, current_thread, next_prioq);
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

	if (mutex->owner == NULL)
	{
		mutex->owner = current_thread;
		return 0;
	}

	current_thread->status = WAITING;
	TAILQ_INSERT_TAIL(&sleepq, current_thread, next_sleepq);

	while (mutex->owner != NULL)
	{
		set_next_thread(mutex->owner);
	}

	mutex->owner = current_thread;
	return 0;
}

int thread_mutex_unlock(thread_mutex_t* mutex)
{
	mutex->owner = NULL;

	if(mutex->is_valid == 0){
		return -1;
	}

	if(!TAILQ_EMPTY(&lockq)){
		thread_t next_locked = TAILQ_FIRST(&lockq);
		next_locked->is_in_lockq = 0;
		TAILQ_REMOVE(&lockq, next_locked, next_lockq);
		next_locked->status = READY;
		TAILQ_INSERT_TAIL(&runq, next_locked, next_runq);
	}

	return 1;
}

struct priority_t* default_priority_queue = NULL;

void init_priorityq()
{
	TAILQ_INIT(&priorityq);
	for (int i = HIGHEST_PRIOTITY; i < LOWEST_PRIORITY; i++)
	{
		struct priority_t* entry = malloc(sizeof(struct priority_t));
		entry->priority = i;
		TAILQ_INIT(&entry->threads);
		TAILQ_INSERT_TAIL(&priorityq, entry, next_priorityq);
		if (i == DEFAULT_PRIORITY) default_priority_queue = entry;
	}
}

void free_priorityq()
{
	struct priority_t* entry = TAILQ_FIRST(&priorityq), * next;
	while (entry != NULL)
	{
		next = TAILQ_NEXT(entry, next_priorityq);
		free(entry);
		entry = next;
	}
}

void add_thread_to_default_priorityq(thread_t thread)
{
	TAILQ_INSERT_TAIL(&default_priority_queue->threads, thread, next_prioq);
	//add_thread_to_priorityq(thread, DEFAULT_PRIORITY);
	//printf("Adding thread %p to default priority queue %d\n", thread, default_priority_queue->priority);
}

struct priority_t* add_thread_to_priorityq(thread_t thread, int priority)
{
	struct priority_t* entry = TAILQ_FIRST(&priorityq);
	while (entry != NULL)
	{
		if (entry->priority == priority)
		{
			TAILQ_INSERT_TAIL(&entry->threads, thread, next_prioq);
			//printf("Adding thread %p to priority queue %d\n", thread, entry->priority);
			return entry;
		}
		entry = TAILQ_NEXT(entry, next_priorityq);
	}
	return NULL;
}

void lower_thread_priority(thread_t thread)
{
	struct priority_t* current_priority_queue = thread->prioq_head;
	int current_priority = current_priority_queue->priority;
	//printf("Lowering thread priority of %p from %d to %d\n", thread, current_priority, current_priority - 1);
	if (current_priority == LOWEST_PRIORITY) return;

	TAILQ_REMOVE(&current_priority_queue->threads, thread, next_prioq);
	//printf("Removing thread %p from priority queue %d\n", thread, current_priority);
	thread->prioq_head = add_thread_to_priorityq(thread, current_priority + 1);
}

void increase_thread_priority(thread_t thread)
{
	struct priority_t* current_priority_queue = thread->prioq_head;
	int current_priority = current_priority_queue->priority;
	//printf("Increasing thread priority of %p from %d to %d\n", thread, current_priority, current_priority - 1);
	if (current_priority == HIGHEST_PRIOTITY) return;

	TAILQ_REMOVE(&current_priority_queue->threads, thread, next_prioq);
	//printf("Removing thread %p from priority queue %d\n", thread, current_priority);
	thread->prioq_head = add_thread_to_priorityq(thread, current_priority - 1);
}

thread_t get_highest_priority_thread()
{
	struct priority_t* queue = TAILQ_FIRST(&priorityq);
	while (queue != NULL)
	{
		//printf("Processing priority queue %d\n", queue->priority);
		// Remove the first thread from the queue that has his status READY
		thread_t thread = TAILQ_FIRST(&queue->threads);
		while (thread != NULL)
		{
			//printf("Status of thread %p is %d, priority %d\n", thread, thread->status, queue->priority);
			if (thread->status == READY)
			{
				//printf("Found thread %p with priority %d\n", thread, queue->priority);
				//TAILQ_REMOVE(&queue->threads, thread, next_prioq);
				return thread;
			}
			thread = TAILQ_NEXT(thread, next_prioq);
		}

		queue = TAILQ_NEXT(queue, next_priorityq);
	}
	//printf("Returning NULL\n");
	return NULL;
}
