//
//  Client_Implementation.c
//  Server
//
//  Created by IdrissRio on 28/04/16.
//  Copyright © 2016 RIFT. All rights reserved.
//
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <time.h>
#include "des.h"
#include "mpi.h"
#include "brute_forcing.h"
//#include <libiomp/omp.h>
#include <omp.h>
#include "Client_Implementation.h"
#include <time.h>


void testOffset(unsigned long long int START_OFFSET, unsigned long long int END_OFFSET, byte **MODIFIED_MESSAGE, byte *SUCCESS,int processID){

    //Struct che permette la comunicazione di dati eterogenei.
    int block[4]={1,1,1,1};
    MPI_Datatype types[4]={MPI_LONG_LONG,MPI_LONG_LONG,MPI_DOUBLE,MPI_INT};
    MPI_Aint disallineamentoBIT[4]={offsetof(infoExec,possibileChiave),offsetof(infoExec,processID),offsetof(infoExec,tempo),offsetof(infoExec,tipo)};
    MPI_Type_struct(4, block, disallineamentoBIT, types, &Info_Type);
    MPI_Type_commit(&Info_Type);
    
    //Fine creazione della struct

    infoExec informazioni;
    informazioni.processID=processID;
    informazioni.tipo=1;
    
    long long key_block_length =262144; // lunghezza dei blocchi
    long long num_blocchi = (END_OFFSET - START_OFFSET) / key_block_length;  //Determino il numero di blocchi di offset da analizzare e testare
    long long resto = (END_OFFSET - START_OFFSET) % key_block_length;        //Possibile resto dovuto alla divisione.
    long long current_start_offset = START_OFFSET;
    long long block_counter;
    long long i;
    
    double start;
    
    printf("Processo %lld", informazioni.processID);
    /*-------------------------------------PRIMA PARTE : SCATTER -------------------------------------------------------- */
    
    for (block_counter = 0; block_counter < num_blocchi; ++block_counter){  //Testo i blocchi che hanno un'ampiezza fissata
        start=omp_get_wtime();
#pragma omp parallel for schedule(dynamic, 10000)
        for ( i = current_start_offset; i < current_start_offset + key_block_length; ++i){
            *SUCCESS = try_decrypt(i, *MODIFIED_MESSAGE);                       //Viene effettuato il test sulla chiave.
            if (*SUCCESS == 1){
		printf("\n\n\n\n debug \n\n\n\n");
                informazioni.tempo=omp_get_wtime()-start;
                informazioni.possibileChiave=i;//Se il test ha avuto successo si comunica al server la chiave utilizzata.
                MPI_Send(&informazioni, sizeof(Info_Type), Info_Type, 0, 0, MPI_COMM_WORLD);  //Inviamo al server la chiave
            }
        }
        informazioni.tempo=omp_get_wtime()-start;
        informazioni.possibileChiave=key_block_length;
        MPI_Send(&informazioni, sizeof(Info_Type), Info_Type, 0, 2, MPI_COMM_WORLD); //Non è stata trovata alcuna chiave. Invio a scopo informativo il risultato al Server.
        current_start_offset += key_block_length;                            //Analizziamo un nuovo gruppo di offset.
    }
    
        start=omp_get_wtime();
#pragma omp parallel for schedule(dynamic, 10000)
    for (i = current_start_offset ; i < current_start_offset + resto; ++i){ //Testo le chiavi che hanno generato un resto nella divisione che genera gli offset.
        *SUCCESS = try_decrypt(i, *MODIFIED_MESSAGE);                      //Viene effettuato il test sulla chiave.
        if (*SUCCESS == 1){                                                //Se il test ha avuto successo si comunica al server la chiave utilizzata.
            informazioni.tempo=omp_get_wtime()-start;
            informazioni.possibileChiave=i;//Se il test ha avuto successo si comunica al server la chiave utilizzata.
            MPI_Send(&informazioni, sizeof(Info_Type), Info_Type, 0, 0, MPI_COMM_WORLD);  //Inviamo al server la chiave
        }
    }
    informazioni.tempo=omp_get_wtime()-start;
    informazioni.possibileChiave=resto;
    MPI_Send(&informazioni, sizeof(Info_Type), Info_Type, 0, 2, MPI_COMM_WORLD);

    
    /*-------------------------------------SECONDA PARTE : CODA -------------------------------------------------------- */
    while (1){
        MPI_Send(&informazioni, sizeof(Info_Type),Info_Type, 0, 1, MPI_COMM_WORLD);      //Comunichiamo al server che abbiamo finito la parte scatter e serve un nuovo offset da analizzare.
        MPI_Recv(&START_OFFSET, 1, MPI_LONG_LONG, 0, 3, MPI_COMM_WORLD, MPI_STATUS_IGNORE); //Riceviamo il nuovo offset e lo testiamo
        if(START_OFFSET==-1)                                                //Il valore -1 indica che non ci sono più offset da testare.
        return;
         start=omp_get_wtime(); //Terminiamo l'esecuzione.
#pragma omp parallel for schedule(dynamic, 10000)
       
        for (i = START_OFFSET; i < START_OFFSET+key_block_length; ++i){     //Testo le chiavi ricevute dal server tramite coda.
            *SUCCESS = try_decrypt(i, *MODIFIED_MESSAGE);                   //Viene effettuato il test sulla chiave.
            if (*SUCCESS == 1){                                             //Se il test ha avuto successo si comunica al server la chiave utilizzata.
                informazioni.tempo=omp_get_wtime()-start;
                informazioni.possibileChiave=i;
                MPI_Send(&informazioni,sizeof(Info_Type), Info_Type, 0, 0, MPI_COMM_WORLD); //Inviamo al server la chiave
            }
        }
    }
}


void testOffsetHybridCpu(byte **MODIFIED_MESSAGE, byte *SUCCESS,int processID){
    long long i;
    unsigned long long int START_OFFSET;
    long long key_block_length =16777216;
    int block[4]={1,1,1,1};
    MPI_Datatype types[4]={MPI_LONG_LONG,MPI_LONG_LONG,MPI_DOUBLE,MPI_INT};
    MPI_Aint disallineamentoBIT[4]={offsetof(infoExec,possibileChiave),offsetof(infoExec,processID),offsetof(infoExec,tempo),offsetof(infoExec,tipo)};
    MPI_Type_struct(4, block, disallineamentoBIT, types, &Info_Type);
    MPI_Type_commit(&Info_Type);
    infoExec informazioni;
    informazioni.processID=processID;
    informazioni.tipo=1;
    while (1){
        double start;
        informazioni.possibileChiave=key_block_length;
        MPI_Send(&informazioni,sizeof(Info_Type), Info_Type, 0, 1, MPI_COMM_WORLD);      //Comunichiamo al server che abbiamo finito la parte scatter e serve un nuovo offset da analizzare.
        MPI_Recv(&START_OFFSET, 1, MPI_LONG_LONG, 0, 3, MPI_COMM_WORLD, MPI_STATUS_IGNORE); //Riceviamo il nuovo offset e lo testiamo
        if(START_OFFSET==-1)                                                //Il valore -1 indica che non ci sono più offset da testare.
            return;//Terminiamo l'esecuzione.
            start=omp_get_wtime();
#pragma omp parallel for schedule(dynamic, 10000)
    
        for (i = START_OFFSET; i < START_OFFSET+key_block_length; ++i){     //Testo le chiavi ricevute dal server tramite coda.
            *SUCCESS = try_decrypt(i, *MODIFIED_MESSAGE);                   //Viene effettuato il test sulla chiave.
            if (*SUCCESS == 1){                                             //Se il test ha avuto successo si comunica al server la chiave utilizzata.
                informazioni.tempo=omp_get_wtime()-start;
                informazioni.possibileChiave=i;//Se il test ha avuto successo si comunica al server la chiave utilizzata.
                MPI_Send(&informazioni, sizeof(Info_Type), Info_Type, 0, 0, MPI_COMM_WORLD);  //Inviamo al server la chiave
            }
        }
        informazioni.tempo=omp_get_wtime()-start;
    }

}



