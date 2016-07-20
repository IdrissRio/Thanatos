//
//  Queue_implementation.c
//  Server
//
//  Created by Francesco Pietralunga on 28/04/16.
//  Copyright © 2016 RIFT. All rights reserved.
//

#include <stdlib.h>
#include <stdio.h>
#include "Queue_implementation.h"

extern int verbose_set;
extern int cpu_mode_set;

// QUEUE_LENGHT:= GRANDEZZA MASSIMA CODA
const unsigned long long int QUEUE_LENGTH=65536;

//Funzione usata per calcolare la potenza di un numero
long long int custom_pow(int base, int exp) {
    
    if(exp == 0) return 1;
    else if(exp == 1) return base;
    else return base * custom_pow(base, exp-1);
}

// controlla che la coda non sia vuota
bool is_empty(struct custom_queue *q) {
    if(q->head == NULL && q->tail == NULL) return true;
    else return false;
}

//Funzione usata per accodare un elemento alla coda
void custom_enqueue(const long long int offset, struct custom_queue *cq) {
    
    struct custom_node *node_ptr = malloc(sizeof(struct custom_node)); // creazione nuovo nodo
    
    node_ptr->value = offset;   //imposto il valore dell'offset
    node_ptr->next = NULL;      //imposto il ptr del nodo a NULL
    
    if(is_empty(cq)) {
        cq->empty_queue = false;    //la coda non è più vuota
        cq->head = node_ptr;        //il ptr di testa punta al nuovo nodo
        cq->tail = node_ptr;        //il ptr di coda punta al nuovo nodo
    }
    else {
        cq->tail->next = node_ptr;  //imposto il ptr del penultimo elemento in modo che punti l'ultimo
        cq->tail = node_ptr;        //imposto il ptr di coda
    }
}

//Funzione usata per rimuovere un elemento dalla testa della coda
const long long int custom_dequeue(struct custom_queue *cq) {
    
    struct custom_node *current_node = cq->head;    //puntatore temporaneo all'elemento che verrà ritornato
    unsigned long long int offset;
    
    if(is_empty(cq)) {
        return -2;
    }
    else {
        cq->head = current_node->next;        //il ptr di testa punta ora a quello che era il secondo elemento della coda
        current_node->next = NULL;            //
        offset = current_node->value;         //prepariamo il nodo per essere rimosso dalla coda
        free(current_node);                   //la memoria occupata dal nodo che era in testa viene rilasciata
    }
    
    return offset;
}

// inizializza la coda prima dell'inizio della consegna degli offset.
void queue_init(struct custom_queue *q, const unsigned long long int OFFSET) {
    if(verbose_set){
        printf("Creazione della coda...\n");
    }
    unsigned long long int i = 0;                                   // indice dell'iterazione
    unsigned long long int current_offset = OFFSET;    // offset attuale (parte da 2^55)
    unsigned long long int offset_len;    // ampiezza dell'offset
    if(cpu_mode_set){
        offset_len =  262144;   // ampiezza dell'offset, che soddisfa il vincolo di mod(2^55,x)=0
    }else{
        offset_len = custom_pow(2, 24);
        
    }
    while(i<QUEUE_LENGTH) {
        custom_enqueue(current_offset, q);     // viene inserito nella coda l'offset corrente
        current_offset += offset_len;          // viene aumentato l'offset corrente
        ++i;
    }
    
    if(verbose_set){
        printf("Coda Pronta\n");
 
    }
}

// libera tutte le risorse associate alla coda (i nodi e la coda stessa)
void free_queue(struct custom_queue *cq) {
    printf("Le risorse allocate per la coda verranno ora deallocate...\n");
    struct custom_node *ptr = cq->head;      // creo un puntatore temporaneo
    while(1) {
        if(ptr->next == NULL) {             // siamo all'ultimo elemento della coda:
            free(ptr);                      // libero l'elemento
            cq->head = NULL;                //
            cq->tail = NULL;                //
            ptr = NULL;                     // setto a NULL head,tail e ptr
            break;
        }
        cq->head = ptr->next;               // faccio avanzare il puntatore di testa
        free(ptr);                          // libero la risorsa tramite ptr
        ptr = cq->head;                     // faccio avanzare ptr a head
    }
    
    free(cq);                               // liberati tutti gli elementi della coda, libero la coda.
    printf("Tutte le risorse allocate sono state deallocate correttamente.\n");
}

/* testa il funzionamento della coda
void test(struct custom_queue *q){
   while(1){
	long long res = custom_dequeue(q);
   	printf(" %lld \n", res);
   }
}
*/
