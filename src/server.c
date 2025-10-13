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
#include <signal.h>

#include "errExit.h"
#include "SHA_256.h"
#include "msg.h"
#include "threadOp.h"

#define LEN 256
#define NUM_THREAD 2


char uuid_str[37];// variabile di appoggio per letture/scritture di uuid
char path2ServerFIFO [LEN];
char path2ThreadFIFO [LEN]; 
struct Message msgRead;
struct Message msgWrite;
uuid_t threadId;
struct ThreadData threadData;

typedef struct clientNode{
    uuid_t clientId;
    struct clientNode *next;
} clientNode;

typedef struct node {
    int ticketNumber;
    int status; // 0 = in coda, 1 = in calcolo, 2 = completato
    clientNode *clientHead;
    
    long weight;
    char filePath[LEN];
    char hash[65];


    struct node *next;
} node;


void addClient(clientNode **clientHead, uuid_t clientId){
    if (*clientHead != NULL && uuid_compare((*clientHead)->clientId, clientId) == 0) { // previene l'aggiunta dello stesso client id piu volte nella lista
        fflush(stdout);
        return;
    }
    clientNode *newNode = malloc(sizeof(clientNode));
    if (!newNode) {
        perror("Errore : fallita allocazione memoria per clientNode");
        return;
    }
    uuid_copy(newNode->clientId, clientId);
    newNode->next = NULL;
    
    if (*clientHead == NULL) {
        *clientHead = newNode;
    } else {
        clientNode *current = *clientHead;
        while (current) {
            if(uuid_compare(current->clientId, clientId) == 0){ // previene l'aggiunta dello stesso client id piu volte nella lista
                fflush(stdout);
                free(newNode);
                return;
            }
            if (current->next == NULL) {
                current->next = newNode;
                return;
            }
            current = current->next;
        }
    }

}

void stampaClientList(clientNode **clientHead){
    clientNode *current = *clientHead;
    char uuid_str[37];
    printf("<Server> stampando lista Client : \n");
    fflush(stdout);
    while (current) {
        uuid_unparse(current->clientId, uuid_str);
        printf(" - Client: %s\n", uuid_str);
        fflush(stdout);
        current = current->next;
    }
}

void printNode(node *node){
    printf("┌────────────────────────────────────────────────────────────────────────┐\n");
    printf("Printing Node: \nTicket Number : %d\nStatus : %d\n", node->ticketNumber, node->status);
    stampaClientList(&node->clientHead);
    printf("File Path : %s\nHash : %s\nWeight : %ld\n", node->filePath, node->hash, node->weight);
    printf("└────────────────────────────────────────────────────────────────────────┘\n");
}

void printTicketList(node *head){
    node *current = head;
    printf("<Server> stampando lista Ticket : \n");
    while (current) {
        printNode(current);
        current = current->next;
    }
}

/*
* funzione principale gestione richieste e ticket
* se prima richiesta: assegno ticket -> creo nodo -> restituisco ticket
* se file in coda: cerco nodo -> restituisco ticket originale
* se file finito: cerco nodo -> restituisco ticket originale + hash
*/


int addFileInQueue(node **head, int *ticketCounterSystem, const char *filePath, uuid_t clientId, char *hash, int *ticket) {

    // [0] Check memory allocation
    node *newNode = malloc(sizeof(node));
    if (!newNode) {
        perror("Errore : fallita allocazione memoria per node");
        return -1;
    }

    // [1] Check filepath validity
    if (access(filePath, F_OK) != 0) {
        fprintf(stderr, "<Server> Il file selezionato : %s non esiste\n", filePath);
        free(newNode);
        return -1;
    }

    // [2] Check if file is already in queue | TODO function searchByPath ?
    node *current = *head;

    while (current) {
        if (strcmp(current->filePath, filePath) == 0) {
            //printNode(current);
            free(newNode);
            if (current->status == 2) {
                printf("<Server> Hash per file %s è gia stato computato , lo restituisco\n", filePath);
                strcpy(hash, current->hash);
                *ticket = current->ticketNumber;
                return -3; // already computed
            } else if(current->status == 1) {
                printf("<Server> Hash per file %s è in calcolamento\n", filePath);
                *ticket = current->ticketNumber;
                return -2; // already in queue or in progress
            } else {
                printf("<Server> errore nella gestione dello status\n");
                return -1;
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
    newNode->clientHead = NULL;
    addClient(&newNode->clientHead, clientId);
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

    // return -1 if path is invalid, -2 if file is already in queue, 1 otherwise
    *ticket = *ticketCounterSystem;
    return 1;
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

/*
 * dato ticket Number restituisce in toClone la copia del nodo con dato ticket 
 */
int getTicketById(int ticketNumber, node *head, node *toClone) {  // 0 se success , -1 se fail
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

int getPendingTicket(node *head,int *ticketNumber, char *filePath){
    node *current = head;
    long minWeight;
    int found = 0;
    while (current) {
        if (current->status == 0) {
            if(!found){
                minWeight = current->weight;
                found = 1;
            }
            if(current->weight <= minWeight){ // ("<=") è importante per ripetere meno codice
                minWeight = current->weight;
                *ticketNumber = current->ticketNumber;
                strcpy(filePath, current->filePath);
            }
        }
        current = current->next;
    }
    return found; // returno minweight cosi se fallisce mi da il numero negativo
}


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

void cleanup(){
    printf("<Server> removing FIFO...\n");
    if (unlink(path2ServerFIFO) != 0){
        errExit("unlink serverFIFO failed");
    }
    if (unlink(path2ThreadFIFO) != 0){
        errExit("unlink threadFIFO failed");
    }
}

void handle_sigint(int sig) {
    printf("\n[signal] Ricevuto SIGINT (Ctrl+C)\n");
    cleanup();
    exit(0);
}


int main(int argc, char *argv[]) {
    signal(SIGINT, handle_sigint);
    printf("╔═══════════════════════════════════════╗\n");
    printf("║          Benvenuto nel Server         ║\n");
    printf("╚═══════════════════════════════════════╝\n");
    
    uuid_t serverId;
    //uint8_t hash[32];
    char char_hash[65];
    
    node *head = NULL;

    node *clonedNode = malloc(sizeof(node));
    clientNode *currentClient;

    pthread_t threads[NUM_THREAD];
    int threadDisponibili = 0;
    int filesPending = 0;
    char filePath[LEN];

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
        printf("<Server> FIFO server : %s creata!\n", path2ServerFIFO);
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
        printf("<Server> FIFO threads : %s creata!\n", path2ThreadFIFO);
    }

    
    memcpy(threadData.threadId, threadId, sizeof(uuid_t));
    for (int i = 0; i < NUM_THREAD; i++) {
        // Create the thread and execute the calculateSum function
        if (pthread_create(&threads[i], NULL, threadOp, (void *)&threadData) != 0) {
            errExit("Error creating thread");
        }
        receive(serverId,&msgRead);
        if(msgRead.messageType == 201 && msgRead.status == 200){
            threadDisponibili++;
            fflush(stdout);
        }else{
            errExit("Errore nella ricezione messaggio dal thread");
        }
    }

    while(1) {
        receive(serverId, &msgRead);

        switch (msgRead.messageType) {
            //messaggi client -> server
            case 1:                                        // 1: Client filePathRequest        // assegna al server il path del file su cui calcolare SHA
                int ticketGiven;
                int result = addFileInQueue(&head, &ticketCounterSystem, msgRead.data, msgRead.senderId, char_hash, &ticketGiven);
                msgWrite.messageType = 101;
                memcpy(msgWrite.destinationId, msgRead.senderId, sizeof(uuid_t));
                switch (result){
                    case 1: // ticket aggiunto alla coda
                        msgWrite.status = 200;
                        msgWrite.ticketNumber = ticketGiven;
                        strcpy(msgWrite.data, msgRead.data);
                        send(&msgWrite);        // server manda ack al client

                        // MANDO MESSAGGIO AD UN THREAD CHE COMINCIA AD ELABORARE
                        if (threadDisponibili > 0) {
                            msgWrite.messageType = 106;
                            msgWrite.status = 200;
                            msgWrite.ticketNumber = ticketGiven;
                            memcpy(msgWrite.destinationId, threadId, sizeof(uuid_t));
                            memcpy(msgWrite.data, msgRead.data, sizeof(msgRead.data));
                            send(&msgWrite);
                        }else{
                            filesPending++;
                            printf("<Server> aggiunta del file %s in coda\n", msgRead.data);
                        }
                        break;
                    case -1: // errore
                        msgWrite.status = 404;
                        msgWrite.ticketNumber = ticketGiven;
                        strcpy(msgWrite.data, "errore in ricerca del file");
                        send(&msgWrite);
                        break;
                        break;
                    case -2: // ticket gia in coda
                        msgWrite.status = 405;
                        msgWrite.ticketNumber = ticketGiven;
                        strcpy(msgWrite.data, "path ricevuta, aggiunto a coda richieste");
                        if (getTicketById(ticketGiven, head, clonedNode) == 0) {
                            addClient(&clonedNode->clientHead, msgRead.senderId);
                        }
                        send(&msgWrite);
                        break;   
                    case -3: // ticket gia calcolato
                        msgWrite.status = 406;
                        msgWrite.ticketNumber = ticketGiven;
                        memcpy(msgWrite.data, &char_hash, sizeof(char_hash));
                        send(&msgWrite);
                        break;   
                }
                break;
            case 2:                                                                 // 2: Client Ticket Status Request  // chiede lo stato o la posizione in coda
                // Restituisco lo status del ticket
                memcpy(msgWrite.senderId, serverId, sizeof(uuid_t));
                memcpy(msgWrite.destinationId, msgRead.senderId, sizeof(uuid_t));
                msgWrite.messageType = 102;
                msgWrite.ticketNumber = msgRead.ticketNumber;
                msgWrite.status = 404; // di default lo status è 404 , se il ticket viene trovato viene sovrascritto, se non viene trovato il ticket rimane 404 e da errore "Ticket non trovato"

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
            //messaggi thread -> server
            case 202:                                                                       // thread conferma presa in carico lavoro
                setFileInProgress(head, msgRead.ticketNumber);
                threadDisponibili--;
            break;

            case 203:                                                                       // thread consegna hash calcolato
                // IL THREAD HA TERMINATO IL CALCOLO DI SHA e server lo sta mandando al client
                // salva il risultato nella lista
                setFileHash(head, msgRead.ticketNumber, msgRead.data);
                if (clonedNode) {
                    if (getTicketById(msgRead.ticketNumber, head, clonedNode) == 0) {        // copia in cloneNode il nodo della lista con il ticket corrispondente      
                        currentClient = clonedNode->clientHead;            
                        msgWrite.messageType = 103;
                        msgWrite.ticketNumber = clonedNode->ticketNumber;
                        memcpy(msgWrite.data, clonedNode->hash, sizeof(clonedNode->hash));
                        do{
                            memcpy(msgWrite.destinationId, currentClient->clientId, sizeof(uuid_t));
                            send(&msgWrite);                                                 // risponde a tutti i client interessati al risultato
                            currentClient = currentClient->next;
                        }while(currentClient != NULL);
                    }
                }
                threadDisponibili++;
                if(filesPending>0){ // se rimane qualche file in coda che aspetta di essere eseguito faccio il fetch del file meno pesante e lo mando direttamente al threadù
                    if(getPendingTicket(head,&ticketGiven,filePath)){
                        msgWrite.messageType = 106;
                        msgWrite.status = 200;
                        msgWrite.ticketNumber = ticketGiven;
                        memcpy(msgWrite.destinationId, threadId, sizeof(uuid_t));
                        memcpy(msgWrite.data, filePath, sizeof(filePath));
                        send(&msgWrite);
                        filesPending--;
                    }else{
                        errExit("Errore in ricerca file a peso minimo");
                    }
                }
            break;

        }
    }

    free(clonedNode);
    cleanup();
    exit(0);
}




