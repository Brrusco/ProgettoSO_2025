#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <uuid/uuid.h>
#include <string.h>

#include "errExit.h"
#include "msg.h"

#define LEN 256

typedef struct node {
    int value;
    struct node *next;
} node;


// Aggiunge un intero alla testa della lista
void addIntToList(node **head, int value) {
    node *newNode = malloc(sizeof(node));
    if (!newNode) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }
    newNode->value = value;
    newNode->next = *head;
    *head = newNode;
}

// Rimuove la prima occorrenza di un intero dalla lista
void removeIntFromList(node **head, int value) {
    node *curr = *head, *prev = NULL;
    while (curr) {
        if (curr->value == value) {
            if (prev)
                prev->next = curr->next;
            else
                *head = curr->next;
            free(curr);
            return;
        }
        prev = curr;
        curr = curr->next;
    }
}

// String to int
int stringToInt(const char *str) {
    int value = 0;
    while (*str) {
        value = value * 10 + (*str - '0');
        str++;
    }
    return value;
}


int main(int argc, char *argv[]) {

    printf("hello world - Client\n");

    char clientMessage [LEN];
    struct Message msgRead;
    struct Message msgWrite;
    uuid_t clientId;
    uuid_t serverId;
    char uuid_str[37];
    int scelta;
    int exitCode;

    exitCode = 1;
    node *likedTickets = NULL;

    uuid_generate(clientId);
    memset(serverId, 0, sizeof(uuid_t)); // server id settato a 0

    char path2ServerFIFO [LEN];
    uuid_unparse(serverId, uuid_str);
    snprintf(path2ServerFIFO, sizeof(path2ServerFIFO), "%s%s", baseFIFOpath, uuid_str);

    char path2ClientFIFO [LEN];
    uuid_unparse(clientId, uuid_str);
    snprintf(path2ClientFIFO, sizeof(path2ClientFIFO), "%s%s", baseFIFOpath, uuid_str);



    printf("<Client> opening FIFO %s...\n", path2ClientFIFO);

    // [01] Creo fifo risposte x il srv (01 SRV)
    if (mkfifo(path2ClientFIFO, S_IRUSR | S_IWUSR | S_IWGRP) == -1) {
        if (errno != EEXIST)
            errExit("[ERROR] mkfifo clientFIFO failed");
        else
            printf("<Client> FIFO %s already exists, using it.\n", path2ClientFIFO);
    }
    else
        printf("<Client> FIFO %s created!\n", path2ClientFIFO);

    

    while(1 && exitCode){

        if(fork() == 0){ // apertura figlio in modalita lettura             (ATTENZIONE non sono sicuro fork ==0 sia il filgio potrebbe essere il padre)
            // [06] Lettura della FIFO (03 SRV)
            receive(clientId, &msgRead);

            // Controllo il tipo di messaggio ricevuto
            if (msgRead.messageType == 101 && msgRead.status == 200) {
                addIntToList(&likedTickets, stringToInt(msgRead.data));
            }

            if (msgRead.messageType == 103 ) {
                removeIntFromList(&likedTickets, stringToInt(msgRead.data));
            }
        }


        // MENU | Child process (TODO: lock with mutex the while)

        /*
        OPTIONS:
        1. Richiedi hash di path/to/file.
        2. Richiedi status di fileInCalcolo.
        3. Chiudi il client.
        */ 
        printf("File in elaborazione:\n");
        for(node *curr = likedTickets; curr != NULL; curr = curr->next) {
            printf("◈ Ticket ID: %d\n", curr->value);
        }

        printf("┌─────────────────────────────────────┐\n");
        printf("│ Opzioni:                            │\n");
        printf("│ 1. Richiedi hash di path/to/file.   │\n");
        printf("│ 2. Richiedi status di fileInCalcolo.│\n");
        printf("│ 3. Chiudi il client.                │\n");
        printf("└─────────────────────────────────────┘\n");
        printf("Seleziona un'opzione: ");

        // Necessario un altro semaforo poiche potrebbe avere problemi con la funzione receive() causando un loop
        scanf("%d", &scelta);

        switch (scelta) {
            case 1:
                // [03] Chiedo all'utente la path del file
                printf("<Client> Dammi la Path del File su cui eseguire SHA256: \n");
                scanf("%s", clientMessage);

                // [04] Creo il messaggio da inviare al server
                msgWrite.messageType = 1;
                msgWrite.status = 0;
                memcpy(msgWrite.senderId, clientId, sizeof(uuid_t));
                memcpy(msgWrite.destinationId, serverId, sizeof(uuid_t));
                snprintf(msgWrite.data, sizeof(msgWrite.data), "%s", clientMessage);

                send(&msgWrite);
                break;
            case 2:
                // [03] Richiedi status di fileInCalcolo
                printf("<Client> Inserisci il ticket: \n");
                scanf("%s", clientMessage);

                // [04] Creo il messaggio da inviare al server
                msgWrite.messageType = 2;
                msgWrite.status = 0;
                memcpy(msgWrite.senderId, clientId, sizeof(uuid_t));
                memcpy(msgWrite.destinationId, serverId, sizeof(uuid_t));
                snprintf(msgWrite.data, sizeof(msgWrite.data), "%s", clientMessage);

                send(&msgWrite);
                break;
            case 3:
                // Chiudi il filgio e il client
                printf("<Client> Chiudo il client.\n");
                exitCode = 0;
                break;
            default:
                printf("Opzione non valida.\n");
                break;
        }
        
    }
   
    if (unlink(path2ClientFIFO) != 0)
        errExit("unlink clientFIFO failed");

    exit(0);
}