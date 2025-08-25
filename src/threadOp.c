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

#include "errExit.h"
#include "SHA_256.h"
#include "msg.h"
// Structure for shared data among threads
struct ThreadData {
    uuid_t threadId; // condiviso tra tutti i threa d, e l'id della fifo su cui i thread fanno la receive , va definito dal server prima della creazione del thread
};

void *threadOp(void *arg){
    struct Message msgRead;
    struct Message msgWrite;

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
    memcpy(msgWrite.data, "ok esisto", strlen("ok esisto") + 1);
    send(&msgWrite);                // scrive al server ok esisto

    /*
    while(msgRead.messageType != 104 ){
        receive(threadData->threadId, &msgRead);
        switch (msgRead.messageType) {
            case 106:                   // il server ha inviato una richiesta SHA
                // figlio (entra solo se ticketGive è maggiore di 0)
                memset(hash, 0, sizeof(hash));
                memset(char_hash, 0, sizeof(char_hash));
                digest_file(msgRead.data, hash);
                
                for(int i = 0; i < 32; i++) {
                    sprintf(char_hash + (i * 2), "%02x", hash[i]);
                }
                // invio il risultato al server che poi lo gira a client
                msgWrite.messageType = 105;
                msgWrite.status = 200;
                msgWrite.ticketNumber = msgRead.ticketNumber;
                memcpy(msgWrite.destinationId, serverId, sizeof(uuid_t));
                memcpy(msgWrite.data, char_hash, sizeof(char_hash));
                send(&msgWrite);
            break;
            default:
                errExit("<THREAD> RCV messaggio ricevuto non riconosciuto");
        }
    }
    */
}