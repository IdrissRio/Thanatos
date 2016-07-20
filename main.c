/******************************************************************************

 ****************************************************************************/

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <libiomp/omp.h>
#include <omp.h>
#include "Server_Implementation.h"
#include "Client_Implementation.h"
#include "Queue_implementation.h"
#include "brute_forcing.h"
#include "des.h"




#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define WHT   "\x1B[37m"
#define RESET "\x1B[0m"


extern int cpu_mode_set;
extern int gpu_mode_set;
extern int hybrid_mode_set;
extern int zcmem_opt_set;
extern int verbose_set;
extern int key_opt_set;

extern long long user_key;

void testOffsetGpu(byte **, byte *,int );
void testOffsetGpuJetson(byte *, byte *,int );

// -------------------Variabili Comuni alle due metà ----------------------//
int processNumber=0;                                        // processNumber := Numero di processi globale
unsigned long long ENCRYPTION_KEY= 512; //2305843009213693951;                       // chiave utilizzata per cifrare il messaggio da decriptare.
                               // Byte:= definizione di un nuovo tipo byte.
byte MESSAGE_CONST[] = "il messaggio segreto e': Lorem ipsum dolor sit amet, malesuada accumsan. Nibh erat pellentesque ultrices, quis odio justo curabitur in montes nunc, laoreet etiam amet, ultricies libero eros lacus potenti nunc, nisl vestibulum sit commodo dolor donec. Ad molestie orci orci vivamus elementum est. Consequat nulla mi mauris, amet dui dictumst enim erat, feugiat posuere molestie nunc, ac maecenas, mauris consequat magna placerat platea. Urna vestibulum, facilisis non sagittis, quam sollicitudin praesent massa sit volutpat. Sed duis, luctus aenean blandit vivamus, eget consectetuer eu sed ante ac. Et turpis ultrices ornare, turpis turpis ac urna velit rutrum ut. Ut wisi, dapibus auctor, molestie enim nulla sagittis ante vestibulum nunc, diam magna ac sagittis accumsan. Sodales ut commodo vehicula, sapien fusce nullam nec quis dui est. Varius pede lacinia suspendisse erat fusce dui.\0";          // Messaggio da criptare/decriptare.
int MESSAGE_SIZE = 1;                                                    // lunghezza del messaggio
byte SUCCESS = 0;                                           // Variabile utilizzata per determinare se è staa ricevuta correttamente la chiave.
byte *MODIFIED_MESSAGE = NULL;                              // Variabile che conterra' il valore del messaggio criptato
unsigned long long int key_counter = 0;                     // tiene conto delle chiavi calcolate fino ad ora dai client
float current_percentage = 0;                               // tiene conto della percentuale delle chiavi calcolate fino ad ora


// -------------------Variabili Prima Metà----------------------//
const unsigned long long int MAX_KEY=72057594037927935;     // MAX_KEY:= 2^56, MASSIMA CHIAVE DELL'ARRAY
unsigned long long int LENGTH_OFFSET_ARRAY=0;               // LENGHT_OFFSET_ARRAY:= LUNGHEZZA MASSIMA ARRAY INIZIALE

// -------------------Variabili Seconda Metà----------------------//
const unsigned long long int START_SECOND_OFFSET=36028797018963968;           // START_SECOND_OFFSET:= Inzio della seconda parte dell'algoritmo, quella gestita dalla coda (2^55 CHIAVI)
long long int last_queue_elem;                                                // Ultimo elemento della coda (viene inizializzato solo dopo l'inizializzazione della coda)
const long long int LENGTH_INTER_QUEUE= 16777216;

// -------------------MPI tags usati durante la comunicazione server - workers----------------------//

const int key_found_tag = 0;                            // tag per notificare la scoperta di una possibile chiave
const int new_offset_blocking_request_tag = 1;          // tag per la ricezione di una MPI_Send bloccante
const int new_offset_non_blocking_request_tag = 2;      // tag per la ricezione di una MPI_Send non bloccante
const int new_offset_to_client_tag = 3;                 // tag per inviare ad un client il nuovo offset (solo dopo MPI_Scatter)
const int key_found_stop=-1;

// -------------------Argomenti riga di comando----------------------//



int main(int argc, char** argv) {
    
    FILE *cpuFile = NULL;
    FILE *gpuFile = NULL;
    FILE *jetsonFile=NULL;
    FILE *hybridFile = NULL;
    FILE *tipoFile=NULL;
    FILE *tempiFile=NULL;
    FILE *datiFinali=NULL;
   
    
    int processID;                                                                  // ID del processo
    extern int processNumber;                                                       // numero dei processi coinvolti
    const int processRoot=0;                                                        // rank del processo padre
    struct custom_queue *main_queue = malloc(sizeof(struct custom_queue));          // istanzia in memoria spazio per la coda degli offset
    //extern unsigned long long int LENGTH_OFFSET_ARRAY;                              // lunghezza dell'array degli offset (MPI_Scatter)
    unsigned long long int OFFSET_ARRAY_CLIENT[2];                                  // Array di due elementi (offset inizio e offset fine) da inviare ai client)
    // OFFSET_ARRAY:= ARRAY CONTENTENTE GLI OFFSET
    MPI_Init(&argc, &argv);                                                         // Inizializzazione ambiente MPI
    MPI_Comm_size(MPI_COMM_WORLD, &processNumber);                                  // Numero totale dei processori
    MPI_Comm_rank(MPI_COMM_WORLD, &processID);                                      // Identifichiamo il processo corrente
    MESSAGE_SIZE = getMessageLenght(MESSAGE_CONST);                                 // inizializzo la lunghezza del messaggio
    
    /********************** LEGGO LE OPZIONI DELL'UTENTE **********************/
    //ora ci interessa solo sapere se l'utente ha specificato l'opzione -key
    //le altre opzioni verranno gestite più avanti (in base al rank del processo)
	
	int code = set_options(argc, argv);
	if(key_opt_set) 
		ENCRYPTION_KEY = user_key;
        
    /**************************************************************************/
    encryptMessage(MESSAGE_CONST, MESSAGE_SIZE, ENCRYPTION_KEY,&MODIFIED_MESSAGE);  //Questa funzione deve essere fatta da tutti i processi altrimente non vengono modificate le variabili.
    LENGTH_OFFSET_ARRAY=(processNumber)*2;
    
    //Struct che permette la comunicazione di dati eterogenei.
    int block[4]={1,1,1,1};
    MPI_Datatype types[4]={MPI_LONG_LONG,MPI_LONG_LONG,MPI_DOUBLE,MPI_INT};
    MPI_Aint disallineamentoBIT[4]={offsetof(infoExec,possibileChiave),offsetof(infoExec,processID),offsetof(infoExec,tempo),offsetof(infoExec,tipo)};
    MPI_Type_struct(4, block, disallineamentoBIT, types, &Info_Type);
    MPI_Type_commit(&Info_Type);
    MPI_Type_commit(&Info_Type);
    
    //Fine creazione della struct
    
    unsigned long long int OFFSET_ARRAY[LENGTH_OFFSET_ARRAY];
    
    //Gestione e analisi dei dati
    if(cpu_mode_set)
        cpuFile=fopen("cpuFile.txt","w");
    if(gpu_mode_set)
        gpuFile=fopen("gpuFile.txt","w");
    if(zcmem_opt_set)
        jetsonFile=fopen("jetsonFile.txt","w");
    if(hybrid_mode_set)
        hybridFile=fopen("hybridFile.txt","w");
    tipoFile=fopen("tipoFile.txt","w");
    tempiFile=fopen("tempiFile.txt","w");
    datiFinali=fopen("datiFinali.txt","w");
    //Fine gestione e analisi dei dati
    
   	if(processID==0){
	welcomeMessage(); 
       if(code == -1) {
            printf("\n\nLe opzioni specificate non sono corrette. Il programma terminera'.\n");
            MPI_Abort(MPI_COMM_WORLD, code);
        } 
       else if(code == 1) {
            show_help_message();
            MPI_Abort(MPI_COMM_WORLD, code);
        }
        if(verbose_set){
            printf(BLU"\n\n####### OPZIONI DI AVVIO #######"RESET);
            printf("\nVerbosita' ATTIVA.");
            /* CHECKING OPTIONS */
            if(cpu_mode_set) {
                printf("\nCPU_mode ATTIVA. ");
            }
            else if(gpu_mode_set){
                printf("\nGPU_mode ATTIVA. ");
            }
            else
                printf("\nHybrid_mode ATTIVA. ");
            if(zcmem_opt_set)
                printf("\nZero_Copy_Memory_option ATTIVA. ");
            
            printf("\n\nMessaggio in chiaro:"WHT" %s"RESET,MESSAGE_CONST);
            printf("\n\nChiave utilizzata per cifrare: "WHT"%llu"RESET,ENCRYPTION_KEY);
            printf(BLU"\n\n####### FINE OPZIONI DI AVVIO #######"RESET);
            fflush(stdout);
        }
        
        
        /* ----- Fase di precomputazione ----- */
        if(verbose_set){
            printf(BLU"\n\n####### INIZIO FASE DI PRECOMPUTAZIONE #######"RESET);
            printf("\n\nMessaggio cifrato ASCII:"WHT);
            
            for(int i=0;i<MESSAGE_SIZE;i++)
                printf(WHT"%c"RESET,MODIFIED_MESSAGE[i]);
            printf("\n\n");
            

            fflush(stdout);
        }
        
        initOffsetArray(OFFSET_ARRAY,LENGTH_OFFSET_ARRAY,START_SECOND_OFFSET);    //Inizializzo l'array con gli offset (dalla chiave 0 alla chiave 2^55 - 1)
        main_queue->empty_queue = true;                                           // inizializzazione della coda (dalla chiave 2^55 alla chiave 2^56 - 1)
        main_queue->head = NULL;                                                  //
        main_queue->tail = NULL;
        if(cpu_mode_set) {
            queue_init(main_queue, custom_pow(2,55));
        }
        else if(gpu_mode_set || hybrid_mode_set) {
            queue_init(main_queue, 0);
        }
        last_queue_elem = main_queue->tail->value;                                // inizializzo l'ultimo elemento della coda (variabile globale)
        /* ----- fine fase di precomputazione ----- */
        if(verbose_set){
            printf(BLU"######## FINE FASE DI PRECOMPUTAZIONE ########\n"RESET);
            fflush(stdout);
        }
    }
    if(verbose_set){
        printf("Distribuzione chiavi ai Client ... \n");
    }
    if(cpu_mode_set)
        MPI_Scatter(OFFSET_ARRAY, 2, MPI_UNSIGNED_LONG_LONG, OFFSET_ARRAY_CLIENT, 2, MPI_UNSIGNED_LONG_LONG,processRoot,MPI_COMM_WORLD);
    
// GESTIONE DEI TAG.
    if(processID==0) {
        long double TempoTotale=0.0;
        long long int chiaviProcessateCPU=0;
        long long int chiaviProcessateGPU=0;
        char * tipoElaboratore=malloc(sizeof(char)*4);
        infoExec client_data;// conserverà il messaggio inviato dal client (la chiave, se è stata trovata)
        while(true) {
            MPI_Status recv_status;                                     // struttura che contiene informazioni sul messaggio ricevuto
            MPI_Recv(&client_data, sizeof(Info_Type), Info_Type, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &recv_status);   //ricezione messaggio
            if(client_data.tipo==0)
                strcpy(tipoElaboratore, "gpu");
            else
                strcpy(tipoElaboratore,"cpu");
            int recieved_tag = recv_status.MPI_TAG;                     // viene ricavato il tag del messaggio inviato
            int response_destination = recv_status.MPI_SOURCE;          // viene ricavato il rank del mittente del messaggio
            bool empty_queue = false;                                   // indica se la coda si è svuotata o no. Permette di uscire da while
            switch(recieved_tag) {                                      // in base al tag ricevuto, eseguiamo un'operazione differente
                case 0: {
                    full_decrypt(client_data.possibileChiave, MODIFIED_MESSAGE, MESSAGE_SIZE);
                    if(verbose_set){
                        printf("\n\n Tutti i Client hanno smesso di elaborare le chiavi e hanno rilasciato le risorse utilizzate. Il processo Server terminerà\n\n\n");
                    }
                    fflush(stdout);
                    fprintf(datiFinali,"TEMPO TOTALE D'ESECUZIONE: %Lf \n",TempoTotale);
                    fprintf(datiFinali,"CHIAVI ELABORATE DA CPU: %llu\n", chiaviProcessateCPU);
                    fprintf(datiFinali,"CHIAVI ELABORATE DA GPU: %llu\n", chiaviProcessateGPU);
                    fprintf(datiFinali,"NUMERO DI CHIAVI PROCESSATE: %llu\n", key_counter);
                    fprintf(datiFinali,"CHIAVE TROVATA: %llu\n", client_data.possibileChiave);
                    if(cpu_mode_set){
                        fflush(cpuFile);
                        fclose(cpuFile);
                    }
                    if(gpu_mode_set){
                        fflush(gpuFile);
                        fclose(gpuFile);
                    }
                    if(zcmem_opt_set){
                        fflush(jetsonFile);
                        fclose(jetsonFile);
                    }
                    if(hybrid_mode_set){
                        fflush(hybridFile);
                        fclose(hybridFile);
                    }
                    fclose(tipoFile);
                    fclose(tempiFile);
                    fclose(datiFinali);
                    
                    free_queue(main_queue);             // libero tutte le risorse allocate per la coda
                    MPI_Abort(MPI_COMM_WORLD, 0);
                    break;
                }
                case 1: {
                    long long int new_offset = custom_dequeue(main_queue);
                    if(new_offset == -2) {
                        printf("ERRORE: La coda e' vuota, impossibile eseguire dequeue.");                      // -2 è il valore di ritorno per quando la coda è vuota
                        exit(-2);
                    }
                    if(new_offset == -1) {
                        printf("\nLa coda degli offset e' vuota. Attendere la ricezione della chiave...\n");
                        empty_queue = true;
                        MPI_Send(&new_offset, 1, MPI_LONG_LONG, response_destination, new_offset_to_client_tag, MPI_COMM_WORLD);
                        break;
                    }
                    
                    
                    MPI_Send(&new_offset, 1, MPI_LONG_LONG, response_destination, new_offset_to_client_tag, MPI_COMM_WORLD);
                    long long int new_offset_enqueued = last_queue_elem + LENGTH_INTER_QUEUE;
                    if( new_offset_enqueued >= MAX_KEY || last_queue_elem == -1) // MAX_KEY = 2^56
                        new_offset_enqueued = -1;                  // se siamo arrivati all'offset 2^(56)-1 abbiamo finito gli offset e bsogna mettere -1 nella coda
                    custom_enqueue(new_offset_enqueued, main_queue);
                    TempoTotale+=client_data.tempo;
                    printf("\n"RED"Percentuale Completamento:"WHT" %.10f%c"RED ". Testate: "WHT"%llu"RED". Rimanenti: "WHT"%llu"RED". Elaborate dal nodo numero:"WHT"%llu - %s" RED". Tempo:"WHT"%f"RED". TOTALE:"WHT" %Lf"RESET, current_percentage,'%',key_counter,MAX_KEY-key_counter,client_data.processID,tipoElaboratore,client_data.tempo,TempoTotale);
                    fflush(stdout);
                    key_counter += client_data.possibileChiave;
                    double percentage =(key_counter*100)/(float)MAX_KEY;
                    current_percentage = percentage;
                    last_queue_elem = new_offset_enqueued;
                    if(client_data.tipo==0) //gpu
                        chiaviProcessateGPU+=client_data.possibileChiave;
                    else
                        chiaviProcessateCPU+=client_data.possibileChiave;
                    if(cpu_mode_set)
                        writeDataOnTxt(cpuFile, tipoFile, tempiFile, client_data.tipo, client_data.tempo, client_data.possibileChiave);
                    else if(gpu_mode_set){
                        writeDataOnTxt(gpuFile, tipoFile, tempiFile, client_data.tipo, client_data.tempo, client_data.possibileChiave);
                    }else if(hybrid_mode_set){
                        writeDataOnTxt(hybridFile, tipoFile, tempiFile, client_data.tipo, client_data.tempo, client_data.possibileChiave);
                    }
                    
                    break;
                    
                }
                case 2: {
                    key_counter += client_data.possibileChiave;
                    double percentage =(key_counter*100)/(float)MAX_KEY;
                    current_percentage = percentage;
                    TempoTotale+=client_data.tempo;
                    printf("\n"RED"Percentuale Completamento:"WHT" %.10f%c"RED ". Testate: "WHT"%llu"RED". Rimanenti: "WHT"%llu"RED". Elaborate dal nodo numero:"WHT"%llu - %s" RED". Tempo:"WHT"%f"RED". TOTALE:"WHT" %Lf"RESET, current_percentage,'%',key_counter,MAX_KEY-key_counter,client_data.processID,tipoElaboratore,client_data.tempo,TempoTotale);
                    fflush(stdout);
                    if(client_data.tipo==0) //gpu
                        chiaviProcessateGPU+=client_data.possibileChiave;
                    else
                        chiaviProcessateCPU+=client_data.possibileChiave;
                    if(cpu_mode_set)
                        writeDataOnTxt(cpuFile, tipoFile, tempiFile, client_data.tipo, client_data.tempo, client_data.possibileChiave);
                    else if(gpu_mode_set){
                        writeDataOnTxt(gpuFile, tipoFile, tempiFile, client_data.tipo, client_data.tempo, client_data.possibileChiave);
                    }else if(hybrid_mode_set){
                        writeDataOnTxt(hybridFile, tipoFile, tempiFile, client_data.tipo, client_data.tempo, client_data.possibileChiave);
                    }

                    break;
                }
                default: {
                    printf("\nIl tag ricevuto non e' valido.\n");
                }
            }
            if(empty_queue) {
                free_queue(main_queue);             // libero tutte le risorse allocate per la coda
                break;                              // usciamo dal ciclo se la coda è vuota
                
            }
        }
    }
    
    
    if(processID > 0){//Processo client. Inizio a processare la parte di chivi che mi interessa.
        if(cpu_mode_set){
            testOffset(OFFSET_ARRAY_CLIENT[0], OFFSET_ARRAY_CLIENT[1], &MODIFIED_MESSAGE, &SUCCESS, processID);
        }
#ifdef GPU_BY_SCRIPT
        if(gpu_mode_set){
            if(zcmem_opt_set){
		testOffsetGpuJetson(MODIFIED_MESSAGE, &SUCCESS, processID);
            }else{
                testOffsetGpu(&MODIFIED_MESSAGE, &SUCCESS, processID);
            }
        }
        
        if(hybrid_mode_set) {
            if(processID%2==1)
                testOffsetHybridCpu(&MODIFIED_MESSAGE, &SUCCESS, processID);
            else if(processID%2==0)
                if(zcmem_opt_set)
                    testOffsetGpuJetson(MODIFIED_MESSAGE, &SUCCESS, processID);
                else
                    testOffsetGpu(&MODIFIED_MESSAGE, &SUCCESS, processID);
                    
        }
        
#endif
        if(verbose_set){
            printf("Il processo %d ha smesso di elaborare.\n",processID);
        }
    }
 
    MPI_Finalize();
}

















	













