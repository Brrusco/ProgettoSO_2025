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
#include <pthread.h>

#include "errExit.h"
#include "SHA_256.h"
#include "msg.h"
#include "threadOp.h"

#define LEN 256

#define NUM_THREAD 10

typedef struct node {
    int ticketNumber;
    int status; // 0 = in coda, 1 = in calcolo, 2 = completato
    uuid_t clientId;
    
    long weight;
    char filePath[LEN];
    char hash[65];


    struct node *next;
} node;

int addFileInQueue(node **head, int *ticketCounterSystem, const char *filePath, uuid_t clientId, char *hash) {

    // [0] Check memory allocation
    node *newNode = malloc(sizeof(node));
    if (!newNode) {
        perror("Failed to allocate memory");
        return -1;
    }

    // [1] Check filepath validity
    if (access(filePath, F_OK) != 0) {
        fprintf(stderr, "File %s does not exist\n", filePath);
        free(newNode);
        return -1;
    }

    // [2] Check if file is already in queue | TODO function searchByPath ?
    node *current = *head;
    while (current) {
        if (strcmp(current->filePath, filePath) == 0) {
            free(newNode);
            if (current->status == 2) {
                printf("Hash per file %s è gia stato computato , lo restituisco\n", filePath);
                strcpy(hash, current->hash);
                return -3; // already computed
            } else {
                printf("Hash per file %s è in calcolamento\n", filePath);
                return -2; // already in queue or in progress
            }
        }
        current = current->next;
    }

    // [3] Initialize new node
    (*ticketCounterSystem)++;
    newNode->ticketNumber = *ticketCounterSystem;   // ticket number
    newNode->status = 0;                            // 0 = in coda
    newNode->weight = 0;                            // default weight
    // calcolo il peso del file
    struct stat fileStat;
    if (stat(filePath, &fileStat) == 0) {
        newNode->weight = fileStat.st_size;
    } else {
        perror("stat failed");
    }
    uuid_copy(newNode->clientId, clientId);
    strncpy(newNode->filePath, filePath, LEN);
    newNode->filePath[LEN - 1] = '\0';              // Ensure null-termination
    newNode->next = NULL;

    if (*head == NULL) {
        *head = newNode;
    } else {
        node *current = *head;
        while (current->next) {
            current = current->next;
        }
        current->next = newNode;
    }

    // return -1 if path is invalid, -2 if file is already in queue, ticket number otherwise
    return *ticketCounterSystem;
}

int setFileInProgress(node *head, const int ticketNumber) {
    node *current = head;
    while (current) {
        if (current->ticketNumber == ticketNumber) {
            current->status = 1;
            return 0;
        }
        current = current->next;
    }
    return -1;
}

int setFileHash(node *head, const int ticketNumber, const char *hash) {
    node *current = head;
    while (current) {
        if (current->ticketNumber == ticketNumber) {
            current->status = 2;
            strncpy(current->hash, hash, 65);
            current->hash[64] = '\0';  // Ensure null-termination
            return 0;
        }
        current = current->next;
    }
    return -1;  // File not found
}

int getTicketById(int ticketNumber, node *head, node *toClone) {
    node *current = head;
    while (current) {
        if (current->ticketNumber == ticketNumber) {
            memcpy(toClone, current, sizeof(node));
            return 0;
        }
        current = current->next;
    }
    memset(toClone, 0, sizeof(node));  // Not found, return empty node
    return -1;
}

int getTicketByWeight(node *head, node *toClone) {
    // recupera il ticket con peso maggiore
    node *current = head;
    long maxWeight = -1;
    while (current) {
        if (current->weight > maxWeight) {
            maxWeight = current->weight;
            memcpy(toClone, current, sizeof(node));
        }
        current = current->next;
    }
    return maxWeight == -1 ? -1 : 0;
}

// TODO trovare un modo per dire al figlio la path da lavorare

void printStatus(node *linkedTicketList) {
    printf("┌────────┬──────────────────────────────┐\n");
    printf("│ TICKET │ STATUS\t\t\t│\n");
    if (linkedTicketList != NULL) {
        printf("├────────┴──────────────┬───────────────┤\n");
    }
    for(node *curr = linkedTicketList; curr != NULL; curr = curr->next) {
        printf("│ ◈ Ticket ID: %d\t│ %s\t│\n", curr->ticketNumber, curr->status == 0 ? "In coda" : curr->status == 1 ? "In calcolo" : "Completato");
        if (curr->next != NULL) {
            printf("├───────────────────────┴───────────────┤\n");
        } else {
            printf("└───────────────────────┴───────────────┘\n");
        }
    }
    if (linkedTicketList == NULL) {
        printf("├────────┴──────────────────────────────┤\n");
        printf("│ ◈ Nessun file in elaborazione\t\t│\n");
        printf("└───────────────────────────────────────┘\n");
    }
}

int main(int argc, char *argv[]) {
    printf("╔═══════════════════════════════════════╗\n");
    printf("║          Benvenuto nel Server         ║\n");
    printf("╚═══════════════════════════════════════╝\n");
    
    uuid_t serverId;
    uuid_t threadId;
    char uuid_str[37];
    //uint8_t hash[32];
    char char_hash[65];
    struct Message msgRead;
    struct Message msgWrite;
    char path2ServerFIFO [LEN];
    char path2ThreadFIFO [LEN];   
    
    node *head = NULL;

    pthread_t threads[NUM_THREAD];
    struct ThreadData threadData;
    int threadDisponibili = 0;

    int ticketCounterSystem;
    ticketCounterSystem = 0;

    memset(serverId, 0, sizeof(uuid_t));
    
    msgWrite.status = 0;
    msgWrite.ticketNumber = 0;
    strcpy(msgWrite.data, "");
    memcpy(msgWrite.senderId, serverId, sizeof(uuid_t));
    

    // [01] Crea fd per FIFO
    uuid_unparse(serverId, uuid_str);
    snprintf(path2ServerFIFO, sizeof(path2ServerFIFO), "%s%s", baseFIFOpath, uuid_str);

    if (mkfifo(path2ServerFIFO, S_IRUSR | S_IWUSR | S_IWGRP) == -1) {
        if (errno != EEXIST){
            errExit("mkfifo failed");
        } else {
            printf("<Server> FIFO %s already exists, using it.\n", path2ServerFIFO);
        }
    } else {
        printf("<Server> FIFO %s creata!\n", path2ServerFIFO);
    }
    

    // [02] inizializzazione dei thread
    
    uuid_generate(threadId);
    uuid_unparse(threadId, uuid_str);
    snprintf(path2ThreadFIFO, sizeof(path2ThreadFIFO), "%s%s", baseFIFOpath, uuid_str);

    if (mkfifo(path2ThreadFIFO, S_IRUSR | S_IWUSR | S_IWGRP) == -1) {
        if (errno != EEXIST){
            errExit("mkfifo failed");
        } else {
            printf("<Server> FIFO %s already exists, using it.\n", path2ThreadFIFO);
        }
    } else {
        printf("<Server> FIFO %s creata!\n", path2ThreadFIFO);
    }

    
    memcpy(threadData.threadId, threadId, sizeof(uuid_t));
    for (int i = 0; i < NUM_THREAD; i++) {
        //printf("thread #%d",i+1);
        // Create the thread and execute the calculateSum function
        if (pthread_create(&threads[i], NULL, threadOp, (void *)&threadData) != 0) {
            errExit("Error creating thread");
        }
        receive(serverId,&msgRead);
        if(msgRead.messageType == 201 && msgRead.status == 200){
            threadDisponibili++;
            //printf("\tok\n");
            fflush(stdout);
        }else{
            errExit("Errore nella ricezione messaggio dal thread");
        }
    }

    while(1) {
        receive(serverId, &msgRead);

        switch (msgRead.messageType) {
            case 1:                                        // 1: Client filePathRequest        // assegna al server il path del file su cui calcolare SHA
                // INIZIO LA PROCEDURA E INVIO RISPOSTA AL CLIENT
                int ticketGiven = addFileInQueue(&head, &ticketCounterSystem, msgRead.data, msgRead.senderId, char_hash);
            
                msgWrite.messageType = 101;
                memcpy(msgWrite.destinationId, msgRead.senderId, sizeof(uuid_t));
                strcpy(msgWrite.data, "path ricevuta");
                if (ticketGiven > 0) {
                    // ok in coda
                    msgWrite.status = 200;
                    msgWrite.ticketNumber = ticketGiven;
                    send(&msgWrite);
                } else if (ticketGiven == -1) {
                    // file non esiste
                    msgWrite.status = 404;
                    msgWrite.ticketNumber = ticketGiven;
                    send(&msgWrite);
                    break;
                } else if (ticketGiven == -2) {
                    // file già in coda o in calcolo
                    msgWrite.status = 405;
                    msgWrite.ticketNumber = ticketGiven;
                    send(&msgWrite);
                    break;
                } else if (ticketGiven == -3) {
                    // file già calcolato -> restituisco hash gia calcolato
                    msgWrite.status = 406;
                    msgWrite.ticketNumber = ticketGiven;
                    memcpy(msgWrite.data, &char_hash, sizeof(char_hash));
                    sleep(1);      // hashinng e troppo veloce , lo rallento un po per vededere se funziona lo scheduling
                    send(&msgWrite);
                    break;
                }

                // MANDO MESSAGGIO AD UN THREAD CHE COMINCIA AD ELABORARE
                msgWrite.messageType = 106;
                msgWrite.status = 200;
                if (threadDisponibili > 0) {
                    msgWrite.ticketNumber = ticketGiven;
                    memcpy(msgWrite.destinationId, threadId, sizeof(uuid_t));
                    memcpy(msgWrite.data, msgRead.data, sizeof(msgRead.data));
                }else{
                    // Se non ci sono thread disponibili mando in coda il primo ticket con priorità (peso maggiore)
                    node *clonedNode = malloc(sizeof(node));
                    msgWrite.ticketNumber = getTicketByWeight(&head, clonedNode);
                    if (msgWrite.ticketNumber != -1) {
                        // Se ho trovato un nodo valido, lo uso
                        memcpy(msgWrite.destinationId, threadId, sizeof(uuid_t));
                        memcpy(msgWrite.data, clonedNode->filePath, sizeof(clonedNode->filePath));
                    } else {
                        errExit("Nessun nodo valido trovato E009");
                    }
                }
                send(&msgWrite);

                break;
            case 2:                                                                 // 2: Client Ticket Status Request  // chiede lo stato o la posizione in coda
                // Restituisco lo status del ticket
                memcpy(msgWrite.senderId, serverId, sizeof(uuid_t));
                memcpy(msgWrite.destinationId, msgRead.senderId, sizeof(uuid_t));

                for (node *curr = head; curr != NULL; curr = curr->next) {
                    if (curr->ticketNumber == msgRead.ticketNumber) {
                        msgWrite.status = curr->status;
                        break;
                    }
                }
                if (msgWrite.status == 0) {
                    msgWrite.status = 200;
                    strcpy(msgWrite.data, "In coda");
                } else if (msgWrite.status == 1) {
                    msgWrite.status = 200;
                    strcpy(msgWrite.data, "In calcolo");
                } else if (msgWrite.status == 2) {
                    msgWrite.status = 200;
                    strcpy(msgWrite.data, "Completato");
                } else {
                    msgWrite.status = 404;
                    strcpy(msgWrite.data, "Ticket non trovato");
                }

                send(&msgWrite);

                break;
            case 105:                                                                       // 105: Thread to Server
                // IL THREAD HA TERMINATO IL CALCOLO DI SHA e se lo sta mandando al client
                if (msgRead.status == 200) {
                    // salva il risultato nella lista
                    setFileHash(head, msgRead.ticketNumber, msgRead.data);

                    node *clonedNode = malloc(sizeof(node));
                    if (clonedNode) {
                        if (getTicketById(msgRead.ticketNumber, head, clonedNode) == 0) {                            
                            msgWrite.messageType = 103;
                            msgWrite.ticketNumber = clonedNode->ticketNumber;
                            memcpy(msgWrite.destinationId, clonedNode->clientId, sizeof(uuid_t));
                            memcpy(msgWrite.data, clonedNode->hash, sizeof(clonedNode->hash));
                            send(&msgWrite);
                        }
                        free(clonedNode);
                    }
                    threadDisponibili++;
                }
                if (msgRead.status == 201) {
                    setFileInProgress(head, msgRead.ticketNumber);
                    threadDisponibili--;
                }
            break;

        }
    }

    
    
    /*
    // [04] Eseguo SHA256 sul file ricevuto e converte in stringa
    digest_file(clientMessage, hash);
    for(int i = 0; i < 32; i++) {
        sprintf(char_hash + (i * 2), "%02x", hash[i]);
    }
    printf("<Server> SHA256: %s\n", char_hash);
    */
    

    // [05] Open the FIFO created by Client in write-only mode


    // [c1] Rimozione della FIFO propria
    printf("<Server> removing FIFO...\n");
    if (unlink(path2ServerFIFO) != 0)
        errExit("unlink serverFIFO failed");
    if (unlink(path2ThreadFIFO) != 0)
        errExit("unlink threadFIFO failed");

    exit(0);
}




