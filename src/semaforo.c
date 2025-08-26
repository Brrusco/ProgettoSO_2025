#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <sys/sem.h>
#include <errno.h>

#include "errExit.h"

/*
* @semid id del semaforo su cui fare l'operazione
*/
int wait(int semid) {
    struct sembuf operation;
    
    operation.sem_num = 0;    // Numero del semaforo nel set
    operation.sem_op = -1;          // Decrementa di 1
    operation.sem_flg = SEM_UNDO;   // Annulla operazione se processo termina

    if (semop(semid, &operation, 1) == -1) {
        errExit("<Semaforo> Wait Fallita");
        return -1;
    }
    return 0;
}

/*
* @semid id del semaforo su cui fare l'operazione
*/
int signal(int semid) {
    struct sembuf operation;
    
    operation.sem_num = 0;    // Numero del semaforo nel set
    operation.sem_op = 1;           // Incrementa di 1
    operation.sem_flg = SEM_UNDO;   // Annulla operazione se processo termina

    if (semop(semid, &operation, 1) == -1) {
        errExit("<Semaforo> Signal Fallita");
        return -1;
    }
    return 0;
}


int sem_init(key_t key, int nsems) {
    int semid;
    // Crea il set di semafori
    semid = semget(key, nsems, IPC_CREAT | IPC_EXCL | 0666);
    
    if (semid == -1) {
        if (errno == EEXIST) {
            semid = semget(key, nsems, 0666);       // Set già esistente, ottieni l'ID
        } else {
            errExit("<Semaforo> SemGet Fallita");
            return -1;
        }
    } else {
        // Set appena creato, inizializza i valori
        for (int i = 0; i < nsems; i++) {
            if (semctl(semid, i, SETVAL, 1) == -1) {
                errExit("<Semaforo> SemCtl Fallita");
                return -1;
            }
        }
    }
    
    return semid;
}



