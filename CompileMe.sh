#!/bin/sh

#  CompileAndRun.sh
#  Server
#
#  Created by IdrissRio on 18/05/16.
#  Copyright © 2016 RIFT. All rights reserved.
echo "Controllo MPI ..."
if  [which mpicc >/dev/null] ; then
    echo "MPI correttamente installato."
else
    echo "MPI non è installato. Provo a caricare il modulo."
    COMMAND="module load compat-openmpi-x86_64"
    echo "Eseguo: "$COMMAND
    $COMMAND
    RETURN=$?
    if [ $RETURN == 0 ]; then
        echo "MPI caricato con successo."
    else
        echo "Non sono stato in grado di caricare il modulo"
        echo "Inserisci il nome del modulo per caricare MPI oppure STOP per fermare l'esecuzione esempio: compat-openmpi-x86_64"
        read MPILOAD
        echo "Eseguo il comando: module load "$MPILOAD
        COMMAND="module load"$MPILOAD
        $COMMAND
        RETURN=$?
        while [ $MPILOAD != "STOP" -a $? == 0 ]
        do
            if [ $RETURN == 0 ]; then
                echo "MPI caricato con successo."
                break
            else
                echo "Comando non trovato: STOP per terminare."
                read MPILOAD
                echo "Eseguo il comando: module load "$MPILOAD
                COMMAND="module load"$MPILOAD
                $COMMAND
            fi
        done
    fi
fi
echo "Compilare il codice per eseguire calcoli su CPU o GPU? [C|G]"
read MODE
    while [ $MODE != "C" -a $MODE != "G" ]
    do
        echo "Perfavore, usa C per CPU o G per GPU"
        read MODE
    done
if [ $MODE == "G" ]; then
    if  which nvcc >/dev/null ; then
        GPU_ACTIVE=1
        echo "Nvcc esiste. Inzio Compilazione."
        #Compilatore da utilizzare
        COMP=" nvcc "
        FLAGS=" -O9 -Xcompiler -fopenmp -I/usr/include/compat-openmpi-x86_64/ -arch=sm_20 -c "
        DEPENDENCY=" Client_ImplementationGPU.cu "

        ## Eseguiamo la compilazione del codice cuda
        $COMP$FLAGS$DEPENDENCY
        RETURN=$?
        if [ $RETURN -ne 0 ]; then
            echo "Processo di compilazione NVCC interrotto ! Errore ! Il compilatore ha terminato l'esecuzione con codice "$RETURN
            exit 1
        fi
        #CEseguiamo la compilaizone del codice mpi
        COMP=" mpicc "
        FLAGS=" -L/usr/local/cuda/lib64 -lcudart -std=c99 -lstdc++ -O3 -fopenmp -DGPU_BY_SCRIPT -o  "
        TARGET=" Server.o"
        #Dipedenze
        DEPENDENCY=" main.c Queue_implementation.c Client_Implementation.c Server_Implementation.c des.c brute_forcing.c Client_ImplementationGPU.o "
        COMMAND=$COMP$FLAGS$TARGET
        #Comando per compilare
        $COMMAND$DEPENDENCY
        RETURN=$?
        if [ $RETURN -ne 0 ]; then
            echo "Processo di compilazione MPICC interrotto ! Errore ! Il compilatore ha terminato l'esecuzione con codice "$RETURN
            exit 1
        fi
    else
        echo "NVCC non esiste su questa macchina, lo script terminerà con stato d'errore 1"
        exit 1
    fi
else
#Eseguiamo la compilaizone del codice mpi
    COMP=" mpicc "
    FLAGS="  -std=c99 -lmpi -O3 -fopenmp -o "
    TARGET=" Server.o"
    #Dipedenze
    DEPENDENCY=" main.c Queue_implementation.c Client_Implementation.c Server_Implementation.c des.c brute_forcing.c "
    COMMAND=$COMP$FLAGS$TARGET
    #Comando per compilare
    $COMMAND$DEPENDENCY
    RETURN=$?
    if [ $RETURN -ne 0 ]; then
        echo "Processo di compilazione MPICC interrotto ! Errore ! Il compilatore ha terminato l'esecuzione con codice "$RETURN
        exit 1
    fi
fi
