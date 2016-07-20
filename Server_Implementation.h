//
//  Server_Implementation.h
//  Server
//
//  Created by IdrissRio on 22/04/16.
//  Copyright © 2016 RIFT. All rights reserved.
//

#ifndef Server_Implementation_h
#define Server_Implementation_h
#include <stdio.h>
#include "des.h"
#include "mpi.h"


#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define WHT   "\x1B[37m"
#define RESET "\x1B[0m"



#ifndef DDB_INPUT_ARGS
#define DDB_INPUT_ARGS
//---------------Variabili per contenere i parametri passati all'avvio---------------
extern int cpu_mode;
extern int cpu_mode_set;
extern int gpu_mode;
extern int gpu_mode_set;
extern int verbose;
extern int verbose_set;
#endif

//Funzione che inizializza il primo array che verrà inviato in modalità Scatter.
void initOffsetArray(unsigned long long int ARRAY[], unsigned long long int LENGTH_OFFSET_ARRAY, unsigned long long int START_SECOND_OFFSET);

void encryptMessage(byte MESSAGE_CONST[], const int MESSAGE_SIZE, const unsigned long long int ENCRYPTION_KEY, byte **MODIFIED_MESSAGE);

void welcomeMessage(void);

int set_options(int argc, char **argv);

void show_help_message();

int getMessageLenght(byte *MESSAGE);

void writeDataOnTxt(FILE* mode,FILE* tipo, FILE* tempo, long long int tipoData, double tempoData, long long int chiaviProcessate);

#endif /* Server_Implementation_h */

