#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <valgrind/valgrind.h>

#include "thread.h"
#include "utils.h"


TAILQ_HEAD(queue, thread_struct) runq;
thread_t T_RUNNING = NULL;
thread_t T_MAIN = NULL;

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
	//printf("CAlled free on %p\n", thread_to_free);
    if (thread_to_free != NULL && thread_to_free != T_MAIN) {
        VALGRIND_STACK_DEREGISTER(thread_to_free->valgrind_stackid);
        free(thread_to_free->context->uc_stack.ss_sp);
        free(thread_to_free->context);
        free(thread_to_free);
        thread_to_free = NULL;
    }
}

__attribute__((unused)) __attribute__((constructor)) void initialize_runq()
{
	TAILQ_INIT(&runq);

	thread_t main_thread = malloc(sizeof(struct thread_struct));
	initialize_thread(main_thread, 1);
	TAILQ_INSERT_HEAD(&runq, main_thread, field);

	T_RUNNING = T_MAIN = main_thread;
}


void print_queue()
{
	printf("MAIN: %p\n", T_MAIN);
	/*thread_t thread;
	TAILQ_FOREACH(thread, &runq, field) printf("%s%p%s -> ",
			thread == T_RUNNING ? "[" : "", thread, thread == T_RUNNING ? "]" : "");*/
	thread_t n1 = TAILQ_FIRST(&runq), n2;
	while (n1 != NULL){
		n2 = TAILQ_NEXT(n1, field);
		printf("%s%p%s -> ", n1 == T_RUNNING ? "[" : "", n1, n1 == T_RUNNING ? "]" : "");
		n1 = n2;
	}
	printf("\n");
}

__attribute__((unused)) __attribute__((destructor)) void destroy_runq()
{
	thread_t n1 = TAILQ_FIRST(&runq), n2;
	while (n1 != NULL)
	{
		n2 = TAILQ_NEXT(n1, field);
		free_thread(n1);
		n1 = n2;
	}

	if (T_MAIN != T_RUNNING) free_thread(T_RUNNING);
	free(T_MAIN->context);
	free(T_MAIN);
	T_MAIN = NULL;
	free(mutex_table);
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

thread_t get_next_thread()
{
	thread_t current_thread = thread_self();
	if (current_thread->previous_thread != NULL)
	{
		// Run waiting thread next
		return current_thread->previous_thread;
	}

	if (TAILQ_EMPTY(&runq)) return T_MAIN;

	thread_t next_thread = TAILQ_NEXT(current_thread, field);
	if (next_thread == NULL) next_thread = TAILQ_FIRST(&runq);

	return next_thread;
}

void set_next_thread(thread_t next_thread)
{
	thread_t current_thread = thread_self();
	next_thread->status = RUNNING;
	T_RUNNING = next_thread;
	swapcontext(current_thread->context, T_RUNNING->context);
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
	TAILQ_REMOVE(&runq, current_thread, field);

	if (TAILQ_EMPTY(&runq))
		setcontext(T_MAIN->context);
	else
		thread_yield();

	exit(0);
}

// Here just so 61-mutex.c can compile, although it obviously doesn't yield the correct result
int thread_mutex_init(thread_mutex_t *mutex) {
	int mut_ind = get_mutex_ind(); // Location in table for this new mutex
	if(mutex_table_size == mut_ind){ // lock table is not long enough
		realloc_mutex_table(&mutex_table, &mutex_table_size);
	}
	mutex->owner = NULL;
	mutex->mutex_index = mut_ind;
	mutex_table[mut_ind] = mutex;
	return 0;
}

int thread_mutex_destroy(thread_mutex_t *mutex) {
	mutex->owner = NULL;
	mutex_table[mutex->mutex_index] = NULL;
	return 0;
}

// Btw I definitively don't understand what I've try to do
int thread_mutex_lock(thread_mutex_t *mutex) {
	if(mutex_table[mutex->mutex_index] == NULL){ // The given lock in not valid
		return 0;
	}
	if(mutex->owner == NULL){ // Give the lock
		mutex->owner = thread_self();
		return 1;
	}
	// thread_t current = thread_self();
	// curent->waited_lock = mutex->mutex_index;
	thread_t owner = mutex->owner;
	while(owner->status != TERMINATED){
		thread_t waiting = thread_self();
		T_RUNNING = owner;
		owner->previous_thread = waiting;
		swapcontext(waiting->context, T_RUNNING->context);
	}
	mutex->owner = thread_self();
	return 1;
}

int thread_mutex_unlock(thread_mutex_t *mutex) {
	mutex->owner = NULL;
	return 0;
}
