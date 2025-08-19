#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "errExit.h"
#include "SHA_256.h"
#include "msg.h"

#define LEN 256


int main(int argc, char *argv[]) {
    printf("hello world Server\n");
    
    uint8_t hash[32];
    int ticketSystem;
    char char_hash[65];
    char clientMessage[LEN];
    char *path2ServerFIFO = "./obj/serverFIFO";
    char *path2ClientFIFO = "./obj/clientFIFO";
    uuid_t serverId;
    memset(serverId, 0, sizeof(uuid_t)); // server id settato a 0

    struct Message msg;
    struct Message response= {
        .senderId = serverId,
        .data = "",
        .status = 0
    };

    

    ticketSystem = 0;


    // [01] Crea fd per FIFO
    if (mkfifo(path2ServerFIFO, S_IRUSR | S_IWUSR | S_IWGRP) == -1) {
        if (errno != EEXIST)
            errExit("mkfifo failed");
        else
            printf("<Server> FIFO %s already exists, using it.\n", path2ServerFIFO);
    }
    else
        printf("<Server> FIFO %s creata!\n", path2ServerFIFO);


    // [02] Opening the server FIFO in read-only mode
    printf("<Server> Attendo il client...\n");
    int serverFIFO = open(path2ServerFIFO, O_RDONLY);
    if (serverFIFO == -1)
        errExit("open serverFIFO failed");
    else
        printf("<Server> Aspettando la path del file...\n");



    // [03] Lettura e stampa della FIFO

    while(1) {
        read(serverFIFO, &msg, sizeof(msg));

        if(fork() == 0) {
            // Elaboro messaggio e invio risposta
            switch (msg.messageType) {
                case 101:
                    // Ricevo il path del file dal client e invia il ticket se il percorso è valido
                    
                    // [A] Il percorso è valido?
                        if (access(msg.data, F_OK) == 0) {
                            // [B] SI - Preparo il ticket incrementando il contatore
                            ticketSystem++;
                            response.messageType = 101;
                            response.status = 200;
                            snprintf(response.data, sizeof(response.data), "%d", ticketSystem);
                            printf("<Server> Ticket %d per il file %s\n", ticketSystem, msg.data);
                        } else {
                            // [C] NO - Preparo il messaggio di errore
                            response.messageType = 101;
                            response.status = 404;
                            snprintf(response.data, sizeof(response.data), "File %s non trovato", msg.data);
                            printf("<Server> Errore: %s\n", response.data);
                        }
                    
                    break;
                    case 102: 
                        break;
                    default: break;
            }
        }
    }

    
    
    
    // [04] Eseguo SHA256 sul file ricevuto e converte in stringa
    digest_file(clientMessage, hash);
    for(int i = 0; i < 32; i++) {
        sprintf(char_hash + (i * 2), "%02x", hash[i]);
    }
    printf("<Server> SHA256: %s\n", char_hash);

    

    // [05] Open the FIFO created by Client in write-only mode
    int clientFIFO = open(path2ClientFIFO, O_WRONLY);
    if (clientFIFO == -1)
        errExit("open failed");

    // [06] Mando al Client la conferma di ricevuta messaggio
    char serverMessage[2*LEN];
    snprintf(serverMessage, sizeof(serverMessage), "File: %s \nSHA256: %s", clientMessage, char_hash);
    
    printf("<Server> inviando la risposta : %s\n", serverMessage);
    if (write(clientFIFO, &serverMessage[0], sizeof(serverMessage)) != sizeof(serverMessage))
        errExit("write failed");
    
    // [c0] chiusura delle fifo
    if (close(serverFIFO) != 0)
        errExit("close serverFIFO failed");
    if (close(clientFIFO) != 0)
        errExit("close clientFIFO failed");

    // [c1] Rimozione della FIFO propria
    printf("<Server> removing FIFO...\n");
    if (unlink(path2ServerFIFO) != 0)
        errExit("unlink serverFIFO failed");

    exit(0);
}




