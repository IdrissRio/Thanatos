//
//  Queue_implementation.h
//  Server
//
//  Created by Francesco Pietralunga on 28/04/16.
//  Copyright © 2016 RIFT. All rights reserved.
//

#ifndef Queue_implementation_h
#define Queue_implementation_h

#include <stdio.h>
#include <stdbool.h>



// -------------------Struct della coda e dei nodi----------------------//
struct custom_node {
    struct custom_node *next;
    unsigned long long int value;
};

struct custom_queue {
    struct custom_node *head;   // l'elemento di testa
    struct custom_node *tail;   // l'elemento di coda
    bool empty_queue;           // indica se la coda è vuota
};

// -------------------Handler della coda----------------------//
//Genera la coda da utilizzare per distribuire la seconda metà delle chiavi ai clients
void queue_init(struct custom_queue *q, const unsigned long long int OFFSET);

// -------------------Operazioni sulla coda----------------------//
// Controlla se la coda è vuota
bool is_empty(struct custom_queue *q);
//Aggiunge un elemento in coda
void custom_enqueue(const long long int offset, struct custom_queue *cq);
//Elimina un elemento dalla testa
const long long int custom_dequeue(struct custom_queue *cq);
//Calcola l'esponenziale di un dato numero e ritorna il valore sotto forma di long long int
long long int custom_pow(int base, int exp);
//Libera tutte le risorse allocate per la coda
void free_queue(struct custom_queue *cq);

/*DEBUG
 void test(struct custom_queue *q);
 */
#endif /* Queue_implementation_h */

