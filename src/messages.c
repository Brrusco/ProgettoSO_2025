#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include "msg.h"


void send(struct Message *message){
    char fifoPath[LEN];
    char uuid_str[37];

    printf("inviando la risposta : %s\n", message->data);

    // [1] Recupera la fifo path corretta 
    uuid_unparse(message->destinationId, uuid_str);
    snprintf(fifoPath, sizeof(fifoPath), "%s%s", baseFIFOpath, uuid_str);

    // [2] Scrive nella FIFO se esiste
    if (write(fifoPath, &message, sizeof(message)) != sizeof(message)){
        if (errno == ENOENT) {
            errExit("<Error> FIFO %s non esiste, assicurati che il client sia in esecuzione.\n", fifoPath);
        }else {
            errExit("<Error> write on fifo : %s failed", fifoPath);
        }
    }

    
    // [3] Switch in base al tipo di messaggio
    switch (message->messageType) {
        case 0:
            printf("[DEBUG] Invio richiesta ID client a %s\n", fifoPath);
            break;
        case 1:
            printf("[DEBUG] Invio richiesta filePath a %s\n", fifoPath);
            break;
        case 2:
            printf("[DEBUG] Invio richiesta stato ticket a %s\n", fifoPath);
            break;
        case 3:
            printf("[DEBUG] Invio richiesta di chiusura connessione a %s\n", fifoPath);
            break;
        default:
            fprintf(stderr, "<Error> Tipo di messaggio sconosciuto: %d\n", message->messageType);
            return;
    }
        
}

// memset(my_uuid, 0, sizeof(uuid_t));
void receive(uuid_t idFifo, struct Message *msg){
    int checkLettura;
    char fifoPath[LEN];
    char uuid_str[37];

    // [1] Recupera la fifo path corretta 
    uuid_unparse(idFifo, uuid_str);
    snprintf(fifoPath, sizeof(fifoPath), "%s%s", baseFIFOpath, uuid_str);

    // [2] Legge la fifo e attende i messaggi
    if(read(fifoPath, &msg, sizeof(msg)) != sizeof(msg)){
        if (errno == ENOENT) {
            errExit("<Error> FIFO %s non esiste, assicurati che il client sia in esecuzione.\n", fifoPath);
        }else {
            errExit("<Error> write on fifo : %s failed", fifoPath);
        }
    }

    // [3] Switch in base al tipo di messaggio
    switch (msg->messageType) {
        case 0:
            printf("[DEBUG] Invio richiesta ID client a %s\n", fifoPath);
            break;
        case 1:
            printf("[DEBUG] Invio richiesta filePath a %s\n", fifoPath);
            break;
        case 2:
            printf("[DEBUG] Invio richiesta stato ticket a %s\n", fifoPath);
            break;
        case 3:
            printf("[DEBUG] Invio richiesta di chiusura connessione a %s\n", fifoPath);
            break;
        default:
            fprintf(stderr, "<Error> Tipo di messaggio sconosciuto: %d\n", msg->messageType);
            return;
    }
}