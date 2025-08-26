#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <uuid/uuid.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <sys/mman.h>


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

    // convert only chars between 0-9
    int value = 0;
    while (*str >= '0' && *str <= '9') {
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
    printf("└───────────────────────────────────────┘\n");
}

void printMenu() {
    printf("┌───────────────────────────────────────┐\n");
    printf("│ Opzioni:                              │\n");
    printf("│ 1. Richiedi hash di path/to/file.     │\n");
    printf("│ 2. Richiedi status di fileInCalcolo.  │\n");
    printf("│ 3. Chiudi il client.                  │\n");
    printf("└───────────────────────────────────────┘\n");
    printf("Seleziona un'opzione: \n");
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

    // [01] Creo fifo risposte x il srv (01 SRV)
    if (mkfifo(path2ClientFIFO, S_IRUSR | S_IWUSR | S_IWGRP) == -1) {
        if (errno != EEXIST)
            errExit("[ERROR] mkfifo clientFIFO failed");
        else
            printf("<Client> FIFO %s already exists, using it.\n", path2ClientFIFO);
    }
    else
        printf("<Client> FIFO %s creata!\n", path2ClientFIFO);

    msgRead.messageType = 5; // forzo il menu alla prima iterazione

    while(1 && exitCode){

        printf("\n");
        cont++;
        printf("< Ciclo numero: %d >\n", cont);


        if(fork() == 0) {
            // Stampo il menu

            if (msgRead.messageType != 5) {
                // Il menu è scatenato solo dopo che riceve una scelta.
                // La prima scelta è scatenata artificialmente
                exit(0);
            }

            char scelta[2];
            
            printMenu();

            memcpy(msgWrite.senderId, clientId, sizeof(uuid_t));
            memcpy(msgWrite.destinationId, clientId, sizeof(uuid_t));
            msgWrite.messageType = 5;
            msgWrite.status = 0;

            scanf(" %s", scelta);
            snprintf(msgWrite.data, sizeof(msgWrite.data), "%s", scelta);

            send(&msgWrite);

            // Chiudo il figlio
            exit(0);
        }

        // [06] Lettura della FIFO (03 SRV)
        printStatus(linkedTickets);
        receive(clientId, &msgRead);


        // Controllo il tipo di messaggio ricevuto
        if (msgRead.messageType == 101 && msgRead.status == 200) {      // 101: Server filePath confirm     // ACK dice ok ho ricevuto il path adesso (non?) calcolo lo SHA (non e ancora finito) e restituisce il ticket del processo SHA
            addIntToList(&linkedTickets, msgRead.ticketNumber);
            printStatus(linkedTickets);
        }
        
        if (msgRead.messageType == 103 ) {                              // 3: Client FINACK                 // chiude la connessione tranquillamente
            removeIntFromList(&linkedTickets, msgRead.ticketNumber);
            printf("┌───────────────────────────────────────┐\n");
            printf("│ Hash calcolato:                       │\n");
            printf("│ %s │\n", msgRead.data);
            printf("└───────────────────────────────────────┘\n");
            printStatus(linkedTickets);
        }

        if (msgRead.messageType == 5 ) {                                // 5: Same Client to Same Client    // quando il figlio parla con il processo padre
            scelta = stringToInt(msgRead.data);
            //printf("\nCLIENT: RICEVE LA SCELTA %d\n", scelta);

            /*
            OPTIONS:
                1. Richiedi hash di path/to/file.
                2. Richiedi status di fileInCalcolo.
                3. Chiudi il client.
            */

            switch (scelta) {
                case 1:
                    // [03] Chiedo all'utente la path del file

                    printf("<Client> Dammi la Path del File su cui eseguire SHA256: \n");
                    fgets(clientMessage, sizeof(clientMessage), stdin);
                    // Remove trailing newline if present
                    clientMessage[strcspn(clientMessage, "\n")] = 0;

                    //printf("\nCLIENT: CHIEDE IL FILE %s\n", clientMessage);


                    // [04] Creo il messaggio da inviare al server
                    msgWrite.messageType = 1;
                    msgWrite.status = 0;
                    memcpy(msgWrite.senderId, clientId, sizeof(uuid_t));
                    memcpy(msgWrite.destinationId, serverId, sizeof(uuid_t));
                    snprintf(msgWrite.data, sizeof(msgWrite.data), "%s", clientMessage);

                    send(&msgWrite);
                    scelta = 0;

                    
                    
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
        }



        




        

    }
   
    if (unlink(path2ClientFIFO) != 0)
        errExit("unlink clientFIFO failed");

    exit(0);
}