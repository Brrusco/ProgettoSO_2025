#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <uuid/uuid.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>


#include "errExit.h"
#include "msg.h"


#define LEN 256

typedef struct node {
    int value;
    struct node *next;
} node;

// Aggiunge un intero alla coda della lista
void addIntToList(node **head, int value) {
    node *newNode = malloc(sizeof(node));
    if (!newNode) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }
    newNode->value = value;
    newNode->next = NULL;

    if (*head == NULL) {
        *head = newNode;
    } else {
        node *curr = *head;
        while (curr->next != NULL) {
            curr = curr->next;
        }
        curr->next = newNode;
    }
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

void printStatus(node *linkedTicketList) {
    printf("┌───────────────────────────────────────┐\n");
    printf("│ File in elaborazione:                 │\n");
    for(node *curr = linkedTicketList; curr != NULL; curr = curr->next) {
        printf("│ ◈ Ticket ID: %d\t\t\t│\n", curr->value);
    }
    if (linkedTicketList == NULL) {
        printf("│ ◈ Nessun file in elaborazione\t\t│\n");
    }
    printf("│ ◈ TESTA: %d\t\t\t│\n", linkedTicketList);
    printf("└───────────────────────────────────────┘\n");
}


int main(int argc, char *argv[]) {

    printf("╔═══════════════════════════════════════╗\n");
    printf("║          Benvenuto nel Client         ║\n");
    printf("╚═══════════════════════════════════════╝\n");

    char clientMessage [LEN];
    struct Message msgRead;
    struct Message msgWrite;
    uuid_t clientId;
    uuid_t serverId;
    char uuid_str[37];
    int scelta;
    int exitCode;
    int cont=0;
    int shmid;

    exitCode = 1;
    node *linkedTickets = NULL;

    uuid_generate(clientId);
    memset(serverId, 0, sizeof(uuid_t)); // server id settato a 0

    char path2ServerFIFO [LEN];
    uuid_unparse(serverId, uuid_str);
    snprintf(path2ServerFIFO, sizeof(path2ServerFIFO), "%s%s", baseFIFOpath, uuid_str);

    char path2ClientFIFO [LEN];
    uuid_unparse(clientId, uuid_str);
    snprintf(path2ClientFIFO, sizeof(path2ClientFIFO), "%s%s", baseFIFOpath, uuid_str);
    


     // Crea un segmento di memoria condivisa
    shmid = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget");
        exit(1);
    }
    
    // Attacca il segmento di memoria al processo
    linkedTickets = (struct node*)shmat(shmid, NULL, 0);
    if (linkedTickets == (node*)-1) {
        perror("[ERROR] in creazione memoria condivisa");
        exit(1);
    }





















    


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

        printf("\n");
        cont++;
        printf("<> Ciclo numero: %d\n", cont);
        printf("\n");
        // MENU | Child process (TODO: lock with mutex the while)

        /*
        OPTIONS:
            1. Richiedi hash di path/to/file.
            2. Richiedi status di fileInCalcolo.
            3. Chiudi il client.
        */

        printStatus(linkedTickets);

        printf("┌───────────────────────────────────────┐\n");
        printf("│ Opzioni:                              │\n");
        printf("│ 1. Richiedi hash di path/to/file.     │\n");
        printf("│ 2. Richiedi status di fileInCalcolo.  │\n");
        printf("│ 3. Chiudi il client.                  │\n");
        printf("└───────────────────────────────────────┘\n");
        printf("Seleziona un'opzione: \n");
        scanf(" %d", &scelta);

        switch (scelta) {
            case 1:
                // [03] Chiedo all'utente la path del file
                printf("<Client> Dammi la Path del File su cui eseguire SHA256: \n");
                scanf(" %s", clientMessage);

                // [04] Creo il messaggio da inviare al server
                msgWrite.messageType = 1;
                msgWrite.status = 0;
                memcpy(msgWrite.senderId, clientId, sizeof(uuid_t));
                memcpy(msgWrite.destinationId, serverId, sizeof(uuid_t));
                snprintf(msgWrite.data, sizeof(msgWrite.data), "%s", clientMessage);

                send(&msgWrite);
                scelta = 0;

                if(fork() == 0){ // apertura figlio in modalita lettura             (ATTENZIONE non sono sicuro fork ==0 sia il figlio potrebbe essere il padre)
                    // [06] Lettura della FIFO (03 SRV)
                    receive(clientId, &msgRead);

                    // Controllo il tipo di messaggio ricevuto
                    if (msgRead.messageType == 101 && msgRead.status == 200) {
                        addIntToList(&linkedTickets, stringToInt(msgRead.data));
                        printStatus(linkedTickets);
                    }

                    if (msgRead.messageType == 103 ) {
                        // TORM printf("Rimosso ticket #%d\n",stringToInt(msgRead.data));
                        removeIntFromList(&linkedTickets, stringToInt(msgRead.data));
                    }

                    shmdt(linkedTickets);// stacco memoria condivisa
                    // Chiudo il figlio
                    exit(0);
                }
                break;
            case 2:
                // [03] Richiedi status di fileInCalcolo
                printf("<Client> Inserisci il ticket: \n");
                scanf(" %s", clientMessage);

                // [04] Creo il messaggio da inviare al server
                msgWrite.messageType = 2;
                msgWrite.status = 0;
                memcpy(msgWrite.senderId, clientId, sizeof(uuid_t));
                memcpy(msgWrite.destinationId, serverId, sizeof(uuid_t));
                snprintf(msgWrite.data, sizeof(msgWrite.data), "%s", clientMessage);

                send(&msgWrite);
                scelta = 0;
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
        sleep(1.5);
    }
   
    if (unlink(path2ClientFIFO) != 0)
        errExit("unlink clientFIFO failed");

    // stacco e cancello memoria condivisa
    shmdt(linkedTickets);
    shmctl(shmid, IPC_RMID, NULL);

    exit(0);
}