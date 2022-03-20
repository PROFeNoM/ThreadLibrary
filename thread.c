#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <valgrind/valgrind.h>

#include "thread.h"
#include "utils.h"

#define FIFO_SIZE 10
#define THREAD_STACK_SIZE 64*1024 // CONSTANTE à requestionner §§§§§§§§§ 


/* identifiant de thread
 * NB: pourra être un entier au lieu d'un pointeur si ca vous arrange,
 *     mais attention aux inconvénient des tableaux de threads
 *     (consommation mémoire, cout d'allocation, ...).
 */

thread_t * FIFO = NULL;
thread_t * T_RUNNING;
ucontext_t * ORDO_CONTEXT;
makecontext(ORDO_CONTEXT, (void (*)(void)) thread_yield, 0); // Fixe le context de l'ordonnanceur


/* recuperer l'identifiant du thread courant.
 */
thread_t thread_self(void) {
    return *T_RUNNING;
}

/* creer un nouveau thread qui va exécuter la fonction func avec l'argument funcarg.
 * renvoie 0 en cas de succès, -1 en cas d'erreur.
 */
int thread_create(thread_t *newthread, void *(*func)(void *), void *funcarg){
    newthread = malloc(sizeof(thread_t));
    newthread->next = NULL;
    ucontext_t uc = newthread->context;
    uc.uc_stack.ss_size = THREAD_STACK_SIZE;
    uc.uc_stack.ss_sp = malloc(uc.uc_stack.ss_size);
    uc.uc_link = ORDO_CONTEXT;  // Le principe est qu'à la fin du thread, la main soi redonnée à l'ordonnanceur
    makecontext(&uc, (void (*)(void)) func, 1, funcarg);

    push_last(FIFO, newthread); // Ajout du nouveau thread en fin de fifo
    
    if((newthread == NULL) || (uc.uc_stack.ss_sp == NULL)){
        return -1;
    } else {
        return 0;
    }
}

/* passer la main à un autre thread.
 */
int thread_yield(void){ // N'ENREGISTRE PAS LE CONTEXT DU THREAD SORTANT
    run_next_thread(&FIFO, &T_RUNNING);
    setcontext(T_RUNNING);
    return -1;
}

/* attendre la fin d'exécution d'un thread.
 * la valeur renvoyée par le thread est placée dans *retval.
 * si retval est NULL, la valeur de retour est ignorée.
 */
int thread_join(thread_t thread, void **retval) {
	// NOTE: Do not work (or maybe, idk), since I can't compile shit
	// NOTE: ik there's are useless/too much comms but idc

    // thread to wait
	thread_t* to_wait = &thread;

	// if thread to wait doesn't exist
	if (!to_wait) return -1;

	// waiting (and calling) thread
	thread_t* waiting = &thread_self();
	waiting->status = JOINING;
	to_wait->next = waiting;  // !!!! Find a way to tell the thread to wait to run calling thread after it is done, maybe using an insert in the FIFO idk

	// if thread to wait is waiting another thread
	if (to_wait->status == WAITING) return -1;

	// save retval's addr
	if (retval != NULL) *retval = to_wait->retval;

	// if the thread is done, free et leave
	if (to_wait->status == TERMINATED) {
		free(to_wait->context.uc_stack.ss_sp);
		free(to_wait->context);
		free(to_wait);
		return 0;
	}

	// else, launch the thread
	waiting->status = WAITING;
	push_last(FIFO, waiting);
	run_next_thread(&FIFO, &to_wait);

	swapcontext(waiting->context, to_wait->context);
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
void thread_exit(void *retval) __attribute__ ((__noreturn__)){
	// NOTE: Do not work (or maybe, idk), since I can't compile shit
	// NOTE: ik there's are useless/too much comms but idc

	// current thread to terminate
	thread_t* current = &thread_self();

	// get retval
	current->retval = retval;

	// update status
	current->status = TERMINATED;

	// run next thread
	thread_t* next_thread = pop_first(&FIFO);
	push_last(FIFO, current);
	run_next_thread(&FIFO, &next_thread);
	setcontext(next_thread->context);
	exit(1);
}
