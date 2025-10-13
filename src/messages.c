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

     if(message->messageType !=5 && message->messageType !=105 && message->messageType != 201 && message->messageType != 106){
        //printf("opening fifo on write : %s \n", fifoPath);
     }
    int fifoPointer = open(fifoPath, O_WRONLY);
    if (fifoPointer == -1) {
        printf("[MSG ERROR] errore in openWrite fifo : %s", fifoPath);
        printf("[MSG ERROR] Probabile processo non avviato \n");
        errExit("[MSG ERROR] Send : open fifo failed\n");
    }

    // [3] Scrive nella FIFO se esiste
    if (write(fifoPointer, message, sizeof(struct Message)) != sizeof(struct Message)){
        if (errno == ENOENT) {
            printf("[MSG ERROR] ENOENT in Write fifo : %s", fifoPath);
            errExit("[MSG ERROR] Send : FIFO non esiste, assicurati che il client sia in esecuzione.\n");
        } else {
            printf("[MSG ERROR] errore in Write fifo : %s", fifoPath);
            errExit("[MSG ERROR] Send : write on fifo failed\n");
        }
    }

    
    // [4] Switch in base al tipo di messaggio
    switch (message->messageType) {
        case 0:
            //printf("[MSG DEBUG] Invio richiesta ID client a %s\n", fifoPath);
            break;
        case 1:
            //printf("[MSG DEBUG] Invio richiesta filePath a %s\n", fifoPath);
            break;
        case 2:
            //printf("[MSG DEBUG] Invio richiesta stato ticket a %s\n", fifoPath);
            break;
        case 3:
            printf("[MSG DEBUG] Invio richiesta di chiusura connessione a %s\n", fifoPath);
            break;
        case 5:
            //printf("[MSG DEBUG] SND msg Client <-> Client \n");
            break;
        case 101:
            //printf("[MSG DEBUG] Invio risposta dal server in merito alla richiesta del file\n" );
            break;
        case 102:
            //printf("[MSG DEBUG] Invio risposta dal server in merito al ticket status\n" );
            break;
        case 103:
            //printf("[MSG DEBUG] Invio risposta dal server con hash del file\n");
            break;
        case 104:
            //printf("[MSG DEBUG] Invio server kill thread\n");
            break;
        case 105:
            //printf("[MSG DEBUG] SND msg Server <-> Server \n");
            break;
        case 106: // messaggio che il server usa per comunicare con i thread
            //printf("[MSG DEBUG] SND msg Server <-> Server \n");
            break;
        case 201: // messagio thread ok
            break;
        case 202: // messagio thread confirm job
            break;
        case 203: // messagio thread hash done
            break;
        default:
            fprintf(stderr, "<Error> Send : Tipo di messaggio sconosciuto: %d\n", message->messageType);
            return;
    }
    
    // [5] Chiudo la FIFO (non distrugge)
    if (close(fifoPointer) != 0){
        printf("[MSG ERROR] errore in close Write fifo : %s", fifoPath);
        errExit("[MSG ERROR] Send : close fifo failed");
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
    //printf("[MSG DEBUG] opening fifo on read : %s \n", fifoPath);
    fifoPointer = open(fifoPath, O_RDONLY);
    if (fifoPointer == -1) {
        printf("[MSG ERROR] errore in openRead fifo : %s", fifoPath);
        errExit("[MSG ERROR] Receive : open fifo failed");
    }   
    
    usleep(100);   //IMPORTANTE ogni tanto la open della fifo in read (async) ci mette troppo tempo e quindi poi fa la read su una fifo ancora non aperta, è brutto ma cosi siamo sicuri che la fifo sia aperta
     
    // [3] Legge la fifo e attende i messaggi
    if(read(fifoPointer, msg, sizeof(struct Message)) != sizeof(struct Message)){
        if (errno == ENOENT) {
            printf("[MSG ERROR] ENOENT in Read fifo : %s\n", fifoPath);
            errExit("[MSG ERROR] Receive : FIFO non esiste, assicurati che il client sia in esecuzione.\n");
        } else {
            printf("[MSG ERROR] errore in Read fifo : %s\n", fifoPath);
            errExit("[MSG ERROR] Receive : read on fifo failed");
        }
    }
    
    // [4] Switch in base al tipo di messaggio per il client e il server
    if(msg->status == 404){
        printf("┌────────────────────────────────────────────────────────────────────────┐\n");
        printf("│ %-70s │\n", "Risposta Ricevuta:");
        printf("│ MESSAGE TYPE: %-56d │\n", msg->messageType);
        printf("│ STATUS: %-62d │\n", msg->status);
        (msg->ticketNumber > 0) ? printf("│ TICKET: %-62d │\n", msg->ticketNumber) : (void)0; // se ticket number > 0 lo stampa
        printf("│ DATA: %-64s │\n", msg->data);
        printf("└────────────────────────────────────────────────────────────────────────┘\n");
    }else{
        printf(">MSG %d\n", msg->messageType);
    }
    fflush(stdout);
   
    switch (msg->messageType) {
        case 1:
            //printf("[MSG DEBUG] Ricevuto richiesta filePath a %s\n", fifoPath);
            break;
        case 2:
            //printf("[MSG DEBUG] Ricevuto richiesta stato ticket a %s\n", fifoPath);
            break;
        case 3:
            printf("[MSG DEBUG] Ricevuto richiesta di chiusura connessione a %s\n", fifoPath);
            break;
        case 5:
            //printf("[MSG DEBUG] RCV msg Client <-> Client \n");
            break;
        case 101:
            //printf("[MSG DEBUG] Ricevuto risposta dal server in merito alla richiesta del file\n" );
            break;
        case 102:
            //printf("[MSG DEBUG] Ricevuto risposta dal server in merito al ticket status\n" );
            break;
        case 103:
            //printf("[MSG DEBUG] Ricevuto hash del file\n");
            break;
        case 104:
            //printf("[MSG DEBUG] Ricevuto server kill thread\n");
            break;
        case 105:
            //printf("[MSG DEBUG] Ricevuto msg Server <-> Server \n");
            break;
        case 106: // messaggio che il server usa per comunicare con i thread
            //printf("[MSG DEBUG] Ricevuto msg Server <-> Server \n");
            break;
        case 201: // messagio thread ok
            break;
        case 202: // messagio thread confirm job
            break;
        case 203: // messagio thread hash done
            break;
        default:
            printf("<Error> Receive : Tipo di messaggio sconosciuto: %d\n", msg->messageType);
            break;
    }
        
        
    // [5] Chiudo la FIFO (non distrugge)
    if (close(fifoPointer) != 0){
        printf("[MSG ERROR] errore in close Read fifo : %s", fifoPath);
        errExit("[MSG ERROR] close fifo failed");
    }  
}