#include <stdio.h>
#include <ucontext.h>

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
    thread_t * newthread = malloc(sizeof(thread_t));
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
int thread_join(thread_t thread, void **retval){
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

}
