#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#include "errExit.h"
#include "msg.h"


void send(struct Message *message){
    char fifoPath[LEN];
    char uuid_str[37];

    // [1] Recupera la fifo path corretta 
    uuid_unparse(message->destinationId, uuid_str);
    snprintf(fifoPath, sizeof(fifoPath), "%s%s", baseFIFOpath, uuid_str);

    // [2] Apro la FIFO in scrittura
    printf("opening fifo on write : %s \n", fifoPath);
    int fifoPointer = open(fifoPath, O_WRONLY);
    if (fifoPointer == -1) {
        printf("[ERROR] fifo : %s", fifoPath);
        printf("[ERROR] Probabile processo non avviato \n");
        errExit("[ERROR] Send : open fifo failed\n");
    }

    // [3] Scrive nella FIFO se esiste
    if (write(fifoPointer, message, sizeof(struct Message)) != sizeof(struct Message)){
        if (errno == ENOENT) {
            printf("[ERROR] fifo : %s", fifoPath);
            errExit("[ERROR] Send : FIFO non esiste, assicurati che il client sia in esecuzione.\n");
        } else {
            printf("fifo : %s", fifoPath);
            errExit("[ERROR] Send : write on fifo failed\n");
        }
    }

    
    // [4] Switch in base al tipo di messaggio
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
        case 5:
            printf("[DEBUG] SND msg Client <-> Client \n");
            break;
        case 101:
            printf("[DEBUG] Invio risposta dal server in merito alla richiesta del file\n" );
            break;
        case 103:
            printf("[DEBUG] Invio risposta dal server con hash del file\n");
            break;
        case 105:
            printf("[DEBUG] msg Server <-> Server \n");
            break;
        default:
            fprintf(stderr, "<Error> Send : Tipo di messaggio sconosciuto: %d\n", message->messageType);
            return;
    }
    
    // [5] Chiudo la FIFO (non distrugge)
    if (close(fifoPointer) != 0){
        printf("[ERROR] fifo : %s", fifoPath);
        errExit("[ERROR] Send : close fifo failed");
    }
        
}

// memset(my_uuid, 0, sizeof(uuid_t));
void receive(uuid_t idFifo, struct Message *msg){
    char fifoPath[LEN];
    char uuid_str[37];
    int fifoPointer;
    
    // [1] Recupera la fifo path corretta 
    uuid_unparse(idFifo, uuid_str);
    snprintf(fifoPath, sizeof(fifoPath), "%s%s", baseFIFOpath, uuid_str);
    
    // [2] Apro la FIFO in lettura
    //printf("[DEBUG] opening fifo on read : %s \n", fifoPath);
    fifoPointer = open(fifoPath, O_RDONLY);
    if (fifoPointer == -1) {
        printf("[ERROR] fifo : %s", fifoPath);
        errExit("[ERROR] Receive : open fifo failed");
    }       
     
    // [3] Legge la fifo e attende i messaggi
    if(read(fifoPointer, msg, sizeof(struct Message)) != sizeof(struct Message)){
        if (errno == ENOENT) {
            printf("[ERROR] fifo : %s", fifoPath);
            errExit("[ERROR] Receive : FIFO non esiste, assicurati che il client sia in esecuzione.\n");
        } else {
            printf("fifo : %s", fifoPath);
            errExit("[ERROR] Receive : read on fifo failed");
        }
    }
    
    // [4] Switch in base al tipo di messaggio per il client e il server
    printf("┌───────────────────────────────────────┐\n");
    printf("│ Risposta:\t\t\t\t│\n");
    printf("│ MESSAGE TYPE: \t%d\t\t│\n", msg->messageType);
    printf("│ STATUS: \t\t%d\t\t│\n", msg->status);
    printf("│ DATA: \t\t%s\t\t│\n", msg->data);
    printf("└───────────────────────────────────────┘\n");
    switch (msg->messageType) {
        case 1:
            printf("[DEBUG] Ricevuto richiesta filePath a %s\n", fifoPath);
            break;
        case 2:
            printf("[DEBUG] Ricevuto richiesta stato ticket a %s\n", fifoPath);
            break;
        case 3:
            printf("[DEBUG] Ricevuto richiesta di chiusura connessione a %s\n", fifoPath);
            break;
        case 5:
            printf("[DEBUG] RCV msg Client <-> Client \n");
            break;
        case 101:
            printf("[DEBUG] Ricevuto risposta dal server in merito alla richiesta del file\n" );
            break;
        case 103:
            printf("[DEBUG] Ricevuto hash del file\n");
            break;
        case 105:
            printf("[DEBUG] msg Server <-> Server \n");
            break;
        default:
            printf("<Error> Receive : Tipo di messaggio sconosciuto: %d\n", msg->messageType);
            break;
    }
        
        
    // [5] Chiudo la FIFO (non distrugge)
    if (close(fifoPointer) != 0){
        printf("[ERROR] fifo : %s", fifoPath);
        errExit("[ERROR] close fifo failed");
    }  
}