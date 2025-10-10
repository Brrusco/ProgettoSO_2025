#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <uuid/uuid.h>
#include <sys/ipc.h>
#include <time.h>

#include "errExit.h"
#include "SHA_256.h"
#include "msg.h"
#include "semaforo.h"

// Structure for shared data among threads
struct ThreadData {
    uuid_t threadId; // condiviso tra tutti i threa d, e l'id della fifo su cui i thread fanno la receive , va definito dal server prima della creazione del thread
};

void *threadOp(void *arg){
    struct Message msgRead;
    struct Message msgWrite;

    struct timespec start, end;
    double elapsed;

    uint8_t hash[32];
    char char_hash[65];

    struct ThreadData *threadData = (struct ThreadData *)arg;

    uuid_t serverId;
    memset(serverId, 0, sizeof(uuid_t)); // server id settato a 0

    msgWrite.messageType = 201;
    msgWrite.ticketNumber = 0;
    msgWrite.status = 200;
    memcpy(msgWrite.senderId, threadData->threadId, sizeof(uuid_t));
    memcpy(msgWrite.destinationId, serverId, sizeof(uuid_t));
    memcpy(msgWrite.data, "thread creato", strlen("thread creato") + 1);
    send(&msgWrite);                // scrive al server thread creato

    
    key_t key = 1000;       // chiave dei semafori per i thread
    int semaforoId = sem_init(key,1);       // richiedo un semaforo data la key (in creat : prendi o crea)
    if(semaforoId == -1){
        errExit("<Thread> Get semaforo fallita");
    }
    
    do{
        waitSem(semaforoId);
        receive(threadData->threadId, &msgRead);
        signalSem(semaforoId);
        switch (msgRead.messageType) {
            case 106:                   // 106: Server assegna ad un thread un filepath per calcolo SHA
                printf("<Thread> file ricevuto : %s\n",msgRead.data);
                clock_gettime(CLOCK_MONOTONIC, &start);

                // Comunico al server che inizio a lavorare
                msgWrite.messageType = 202;
                msgWrite.ticketNumber = msgRead.ticketNumber;
                memcpy(msgWrite.data, "thread ticket assign", strlen("thread ticket assign") + 1);
                msgWrite.status = 200;
                send(&msgWrite);                    // conferma presa in carico del ticket
            
                memset(hash, 0, sizeof(hash));
                memset(char_hash, 0, sizeof(char_hash));
                digest_file(msgRead.data, hash);
                
                for(int i = 0; i < 32; i++) {
                    sprintf(char_hash + (i * 2), "%02x", hash[i]);
                }
                // invio il risultato al server che poi lo gira a client
                msgWrite.messageType = 203;
                msgWrite.status = 200;
                msgWrite.ticketNumber = msgRead.ticketNumber;
                memcpy(msgWrite.destinationId, serverId, sizeof(uuid_t));
                memcpy(msgWrite.data, char_hash, sizeof(char_hash));
                sleep(10);      // hashinng e troppo veloce , lo rallento un po per vededere se funziona lo scheduling
                clock_gettime(CLOCK_MONOTONIC, &end);
                elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1000000000.0;

                printf("<Thread> file : %s  - DONE - Hash : %s | Tempo di elaborazione: %.3f secondi\n",msgRead.data,char_hash, elapsed);
                send(&msgWrite);
              
            break;
            default:
                errExit("<Thread> RCV messaggio ricevuto non riconosciuto");
        }
        
    }while(msgRead.messageType != 104 );
}