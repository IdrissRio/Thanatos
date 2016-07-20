//
//  Server_Implementation.c
//  Server
//
//  Created by IdrissRio on 22/04/16.
//  Copyright © 2016 RIFT. All rights reserved.
//

#include <math.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "brute_forcing.h"
#include "Server_Implementation.h"
#include <string.h>

//----------Definizione colori per testo----------
#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define WHT   "\x1B[37m"
#define RESET "\x1B[0m"

//----------Inizializzazione variabili per opzioni---------
int     cpu_mode         = 1;       // abilitata di default
int     cpu_mode_set     = 0;
int     gpu_mode         = 0;
int     gpu_mode_set     = 0;
int     hybrid_mode      = 0;
int     hybrid_mode_set  = 0;
int     key_opt_set      = 0;
int     key_not_set      = 1;
int     zcmem_opt        = 0;
int     zcmem_opt_set    = 0;
int     verbose          = 0;
int     verbose_set      = 0;

long long user_key;                 // la possibile chiave che l'utente vuole immettere

void welcomeMessage(void){
    printf(YEL "#######################################################################################\n" RESET);
    printf(RED "#"RESET BLU"                           DISTRIBUTED DES BRUTEFORCER                               "RESET RED"#\n" RESET);
    printf(RED "#"RESET BLU"                    Copyright © 2016 RIFT. All rights reserved.                      "RESET RED"#\n" RESET);
    printf(RED "#                                                                                     #\n" RESET);
    printf(RED "#"RESET BLU"                                     DEVELOPERS                                      "RESET RED"#\n" RESET);
    printf(RED "#                                                                                     #\n" RESET);
    printf(RED "#"RESET WHT"            Idriss Riouak,   Francesco Pietralunga              (Server Side)        "RESET RED"#\n" RESET);
    printf(RED "#"RESET WHT"            Tommaso Campari, Renato Eugenio Garavaglia          (Client Side)        "RESET RED"#\n" RESET);
    printf(RED "#"RESET WHT"            Tommaso Campari, Francesco Pietralunga           (DES Implementation)    "RESET RED"#\n" RESET);
    printf(YEL "#######################################################################################\n" RESET);
    
}

/************************************************************************************
 
                                    NOTA BENE
 
 Le funzioni per la gestione delle opzioni in ingresso sono FORTEMENTE dipendenti dalle
 opzioni stesse. Se si voglioni aggiungere nuove opzioni sarà NECESSARIO modificare
 le funzioni e le variabili interessate.
 
 ***********************************************************************************/


//---------------GESTIONE OPZIONI IN ENTRATA---------------
char **valid_options = NULL;                                    // array delle opzioni

//---------------GESTIONE OPZIONI IN ENTRATA---------------
// permette di controllare se l'opzione esaminata è contenuta nell'array delle opzioni valide
int contained_in(char *str, char **options_array, int options_array_len) {
    
    for(int i = 0; i<options_array_len; ++i) {
        /*DEBBUG printf("\noptions_array[i]: %s", options_array[i]);*/
        if(!strcmp(str,options_array[i])) {
            /*DEBBUG printf("\ncorrispondono");*/
            return i+1;                                         // ritorna l'indice della corrispondenza con l'array delle opzioni
        }                                                       // aumentato di uno
    }
    return 0;
}

//---------------GESTIONE OPZIONI IN ENTRATA---------------
// in base all'opzione specificata, vengono settate le variabili globali per i parametri
int handle_option(int option_index) {
    /*DEBUG printf("\nvalore option_index: %d", option_index); */
    switch(option_index) {
        case 0: {                                                                                                       //case: 0 (-cpu)
            if(cpu_mode_set) {
                printf("\nErrore: opzione %s gia' specificata\n", valid_options[0]);
                return -1;
            }
            else if(gpu_mode_set) {
                printf("\nErrore: e' gia' stata specificata l'opzione %s.\n", valid_options[1]);
                return -1;
            }
            else if(hybrid_mode_set) {
                printf("\nErrore: e' gia' stata specificata l'opzione %s.\n", valid_options[2]);
                return -1;
            }
            else if(zcmem_opt_set) {
                printf("\nErrore: l'opzione %s necessita di essere specificata solo dopo aver inserito una modalita' valida (%s oppure %s)\n",
                       valid_options[3], valid_options[1], valid_options[2]);
                return -1;
            }
            cpu_mode = 1;
            gpu_mode = 0;
            cpu_mode_set = 1;
            break;
        }
        case 1: {                                                                                                       //case: 1 (-gpu)
            if(gpu_mode_set) {
                printf("\nErrore: opzione %s gia' specificata\n", valid_options[1]);
                return -1;
            }
            else if(cpu_mode_set) {
                printf("\nErrore: e' gia' stata specificata l'opzione %s.\n", valid_options[0]);
                return -1;
            }
            else if(hybrid_mode_set) {
                printf("\nErrore: e' gia' stata specificata l'opzione %s.\n", valid_options[2]);
                return -1;
            }
            else if(zcmem_opt_set) {
                printf("\nErrore: l'opzione %s necessita di essere specificata solo dopo aver inserito una modalita' valida (%s oppure %s)\n",
                       valid_options[3], valid_options[1], valid_options[2]);
                return -1;
            }
            cpu_mode = 0;           //disabilito l'opzione di default
            gpu_mode = 1;
            gpu_mode_set = 1;
            break;
        }
        case 2: {                                                                                                       //case: 2 (-hybrid)
            if(hybrid_mode_set) {
                printf("\nErrore: opzione %s gia' specificata\n", valid_options[2]);
                return -1;
            }
            else if(cpu_mode_set || gpu_mode_set) {
                printf("\nErrore: e' gia' stata specificata una tra le opzioni: %s, %s\n", valid_options[0], valid_options[1]);
                return -1;
            }
            else if(zcmem_opt_set) {
                printf("\nErrore: l'opzione %s necessita di essere specificata solo dopo aver inserito una modalita' valida (%s oppure %s)\n",
                       valid_options[3], valid_options[1], valid_options[2]);
                return -1;
            }
            cpu_mode = 0;           //disabilito l'opzione di default
            hybrid_mode = 1;
            hybrid_mode_set = 1;
            break;
        }
        case 3: {                                                                                                       //case: 3 (-key)
            if(key_opt_set) {
                printf("\nErrore: opzione %s gia' specificata\n", valid_options[3]);
                return -1;
            }
            key_opt_set = 1;
            break;
        }
        case 4: {                                                                                                       //case: 4 (-zcmem)
            if(zcmem_opt_set) {
                printf("\nErrore: opzione %s gia' specificata\n", valid_options[4]);
                return -1;
            }
            else if(cpu_mode || cpu_mode_set) { //cpu è opzione di default, zcmem deve fare il controllo anche se cpu_mode_set è a 0
                printf("\nErrore: la modalita' %s non consente l'utilizzo dell'opzione %s\n", valid_options[0], valid_options[4]);
                //printf("\n(forse hai specificato -zcmem prima di -gpu o -hybrid?)");
                return -1;
            }
            zcmem_opt = 1;
            zcmem_opt_set = 1;
            break;
        }
        case 5: {                                                                                                       //case: 5 (-v)
            if(verbose_set) {
                printf("\nErrore: l'opzione %s e' gia' stata specificata\n", valid_options[5]);
                return -1;
            }
            verbose = 1;
            verbose_set = 1;
            break;
        }
        default: {
            printf("\nErrore: nessuna corrispondenza trovata nelle opzioni. il programma terminera'\n");
            return -1;
        }
    }
    return 0;
}

//---------------GESTIONE OPZIONI IN ENTRATA---------------
// mostra il messaggio di aiuto sull'utilizzo delle opzioni
void show_help_message() {
    
    printf("\nBenvenuti nel menu' delle opzioni di \'Disributed Des Bruteforcer project\'.\n\n");
    printf("Le opzioni disponibili sono:\n");
    printf("\n\t\t -cpu:        questa opzione permette di lanciare il programma in cpu_mode, ovvero distribuira'");
    printf("\n\t\t              i calcoli sui PROCESSORI dei nodi del cluster che verranno utilizzati.");
    printf("\n\t\t              (NB: l'utilizzo di -cpu NON puo' essere simultaneo a -gpu o a -hybrid)");
    printf("\n\t\t              (Abilitata di DEFAULT)");
    printf("\n\t\t -gpu:        questa opzione permette di lanciare il programma in gpu_mode, ovvero distribuira'");
    printf("\n\t\t              i calcoli sulle GPU dei nodi del cluster che verranno utilizzati.");
    printf("\n\t\t              (NB: l'utilizzo di -gpu NON puo' essere simultaneo a -cpu o a -hybrid)\n");
    printf("\n\t\t -hybrid:     questa opzione permette di lanciare il programma in hybrid_mode, ovvero distribuira'");
    printf("\n\t\t              i calcoli sia sui core della/delle CPU che sulle GPU dei nodi del cluster utilizzati.");
    printf("\n\t\t              (NB: l'utilizzo di -hybrid NON puo' essere simultaneo a -cpu o a -gpu)\n");
    printf("\n\t\t -key:        l'opzione permette di specificare la chiave con cui cifrare il messaggio.");
    printf("\n\t\t              Se non viene specificata l'opzione, il testo sara' cifrato con una chiave");
    printf("\n\t\t              di default (5).\n");
    printf("\n\t\t -zcmem:      questa opzione, utilizzabile solo in caso si abbia specificato prima una tra le modalita'");
    printf("\n\t\t              -gpu o -hybrid, specifica che le GPU utilizzate hanno una memoria condivisa con la CPU ");
    printf("\n\t\t              a cui si accede con il metodo 'zero copy'. (l'opzione e' stata pensata per condurre test");
    printf("\n\t\t              specifici sul kit 'Jetson TX1').");
    printf("\n\t\t -h:          l'opzione mostra questo messaggio di aiuto.\n");
    printf("\n\t\t -v:          l'opzione permette di visualizzare un output piu' prolisso durante il bruteforce.");
    printf("\n\n\n");
    return;
}


//---------------GESTIONE OPZIONI IN ENTRATA---------------
int set_options(int argc, char **argv) {
    
    const int valid_options_len = 6;
    valid_options = malloc(sizeof(char *) * valid_options_len); // opzioni valide:
    valid_options[0] = malloc(sizeof(char)*5);                  //
    strcpy(valid_options[0], "-cpu");                           // modalità CPU (no opzioni gpu)
    valid_options[1] = malloc(sizeof(char)*5);                  //
    strcpy(valid_options[1], "-gpu");                           // modalità GPU (no opzioni cpu e hybrid, consentita -zcmem)
    valid_options[2] = malloc(sizeof(char)*8);                  //
    strcpy(valid_options[2], "-hybrid");                        // modalità ibrida (CPU+GPU) (no -gpu o -cpu, consentita -zcmem)
    valid_options[3] = malloc(sizeof(char)*5);
    strcpy(valid_options[3], "-key");
    valid_options[4] = malloc(sizeof(char)*7);                  //
    strcpy(valid_options[4], "-zcmem");                         // modalità GPU con memoria condivisa tra CPU e GPU
    valid_options[5] = malloc(sizeof(char)*3);                  // (solo se una tra -gpu o -hybrid è settata)
    strcpy(valid_options[5], "-v");                             // verbosità
    
    /*DEBBUG printf("\nnumero argomenti: %d", argc);*/
    /*DEBBUG for(int i = 0; i<argc; ++i) printf("\nargv di i:%d e': %s", i, argv[i]);*/
    
    if(argc == 2 && !strcmp(argv[1],"-h")) {
        return 1;
    }
    else if(argc == 2 && !(!strcmp(argv[1],"-cpu") || !strcmp(argv[1],"-gpu") || !strcmp(argv[1], "-hybrid"))) {
        printf("Errore: e' necessario specificare una tra le seguenti modalita' prima di specificare altre opzioni: %s, %s, %s\n",
               valid_options[0], valid_options[1], valid_options[2]);
        return -1;
    }
    else{
        /*DEBBUG printf("\nabbiamo specificato -cpu, -gpu o -hybrid come prima opzione");*/
        for(int i = 1; i<argc; ++i) {
            int option_index = contained_in(argv[i], valid_options, valid_options_len);
            /*DEBBUG printf("\nvalore ritornato: %d", option_index);*/
            if(option_index) {
                /*DEBBUG printf("\nentrati in if");*/
                int error = handle_option(option_index-1);
                if(key_opt_set && key_not_set) {                    // se l'utente inserisce la chiave:
                    user_key = atoll(argv[i+1]);                    // viene memorizzata la chiave (contenuta in argv[i+1]) dentro a user_key
                    key_not_set = 0;                                // aggiorno la variabile che controlla se la chiave è già stata settata
                    ++i;                                            // salto l'indice di argv che contiene la chiave per continuare a fare il parsing delle opt.
                }
                /*DEBBUG printf("\nvalore error ritornato: %d",error); */
                if(error==-1) {
                    //printf("\nErrore: errore nella funzione handle_option\n");
                    return -1;
                }
            }
            else {
                printf("\nErrore: opzione non riconosciuta. il programma terminera'.\n");
                return -1;
            }
        }
    }
    
    /* DEBUG
     printf("\nProgramma avviato senza errori, i valori delle opzioni sono: ");
     printf("\n\tcpu_mode = %d", cpu_mode);
     printf("\n\tgpu_mode = %d", gpu_mode);
     printf("\n\tverbose = %d\n", verbose); 
    */

    for(int i=0; i<valid_options_len; ++i) {
        free(valid_options[i]);
    }
    free(valid_options);
    
    if(cpu_mode) cpu_mode_set = 1; //in caso cpu_mode resti abilitata di default, settiamo anche cpu_mode_set, che altrimenti rimarrebbe a 0
    return 0;
}
    

                                                                                                //Funzione che inizializza il primo array che verrà inviato in modalità Scatter.
void initOffsetArray(unsigned long long int ARRAY[], unsigned long long int LENGTH_OFFSET_ARRAY, unsigned long long int START_SECOND_OFFSET){
    unsigned long long int INTER_FIRST_OFFSET = ceil(START_SECOND_OFFSET/((LENGTH_OFFSET_ARRAY-2)/2));  //Ampiezza di ogni intervallo di chiavi che andremmo a distribire.
    unsigned long long int CHECK_INTER = START_SECOND_OFFSET%((LENGTH_OFFSET_ARRAY-2)/2);               //Variabile di controllo che determina il resto e verrà utilizzata nell'Assert
    if(verbose){
        printf("\nNumero di processi in esecuzione: %llu (1 Server e %llu Client) \n", LENGTH_OFFSET_ARRAY/2,(LENGTH_OFFSET_ARRAY/2)-1);
    }
    ARRAY[0]=0;                                                                                         //Padding per il processo 0 che è il server. Il server non elabora chiavi.
    ARRAY[1]=0;                                                                                         //Padding per il processo 0 che è il server. Il server non elabora chiavi.
    for (int i=2,j=0;i<(LENGTH_OFFSET_ARRAY-2);j++,i+=2){
        //Inizializziamo l'array contenente gli offset.
        ARRAY[i]=INTER_FIRST_OFFSET*j;
        ARRAY[i+1]=ARRAY[i]+(INTER_FIRST_OFFSET-1);
    }
    if((LENGTH_OFFSET_ARRAY-2)/2==1){
        ARRAY[LENGTH_OFFSET_ARRAY-2]=ARRAY[LENGTH_OFFSET_ARRAY-3];                                        //Caso in cui utilizziamo solamente un unico processo per elaborare le chiavi.
        ARRAY[LENGTH_OFFSET_ARRAY-1]=START_SECOND_OFFSET;
  
    }else{
        ARRAY[LENGTH_OFFSET_ARRAY-2]=ARRAY[LENGTH_OFFSET_ARRAY-3]+1;                                      //Caso in cui utilizziamo un numero di processi >= 2
        ARRAY[LENGTH_OFFSET_ARRAY-1]=START_SECOND_OFFSET;
    }
#define NDEBUG
#ifdef NDEBUG
    assert(ARRAY[LENGTH_OFFSET_ARRAY-2]+INTER_FIRST_OFFSET+CHECK_INTER == ARRAY[LENGTH_OFFSET_ARRAY-1] && ARRAY[LENGTH_OFFSET_ARRAY-1]==START_SECOND_OFFSET);
#endif
}


void encryptMessage(byte MESSAGE_CONST[], const int MESSAGE_SIZE, const unsigned long long int ENCRYPTION_KEY, byte **MODIFIED_MESSAGE){
    byte *key = NULL;
    byte **subkeys = NULL;
    byte *message = malloc(MESSAGE_SIZE*BYTE);                         // allocazione della memoria per il messaggio da cifrare. 24 era per le prove con ''il messaggio segreto e':''
    for (int i = 0; i < MESSAGE_SIZE; ++i ){
        message[i] = MESSAGE_CONST[i];
    }
    generate_key_from_longlong_value(ENCRYPTION_KEY, &key, 0);         //generazione stringa binaria da longlong
    get_subkeys(&key, &subkeys, 0);                                    // generazione delle 16 sottochiavi
    cipher_encrypt(subkeys, &message, MESSAGE_SIZE, MODIFIED_MESSAGE); // crittazione
    for(int i=0;i<16;i++)
        free(subkeys[i]);
    free(subkeys);
    
}

int getMessageLenght(byte *MESSAGE){
    int lenght=0;
    char c=MESSAGE[0];
    while(c!='\0'){
        ++lenght;
        c=MESSAGE[lenght];
    }
    return lenght;
}

void writeDataOnTxt(FILE* mode,FILE* tipo, FILE* tempo, long long int tipoData, double tempoData, long long int chiaviProcessate){
    fprintf(mode,"%lld\n",chiaviProcessate);
    if(tipoData==0)
        fprintf(tipo,"GPU\n");
    else
        fprintf(tipo,"CPU\n");
    fprintf(tempo,"%f\n",tempoData);
    
};





