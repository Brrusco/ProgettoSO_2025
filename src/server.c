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

#define LEN 256


int main(int argc, char *argv[]) {
    printf("hello world Server\n");
    
    uuid_t serverId;
    uint8_t hash[32];
    char uuid_str[37];
    char char_hash[65];
    int ticketSystem;
    struct Message msgRead;
    struct Message msgWrite;
    char path2ServerFIFO [LEN];    
    
    ticketSystem = 0;
    memset(serverId, 0, sizeof(uuid_t));
    
    msgWrite.status = 0;
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



    while(1) {
        receive(serverId, &msgRead);

    
        
        if(fork() == 0) {
            printf("[DEBUG] figlio aperto\n");
            // Elaboro messaggio e invio risposta
            switch (msgRead.messageType) {
                case 1:
                    // Ricevo il path del file dal client e invia il ticket se il percorso è valido

                    // [A] Il percorso è valido?
                        memcpy(msgWrite.destinationId, msgRead.senderId, sizeof(uuid_t));
                        msgWrite.messageType = 101;
                        if (access(msgRead.data, F_OK) == 0) {
                            // [B] SI - Preparo il ticket incrementando il contatore
                            ticketSystem++;
                            msgWrite.status = 200;
                            snprintf(msgWrite.data, sizeof(msgWrite.data), "%d", ticketSystem);
                            printf("<Server> Ticket %d per il file %s\n", ticketSystem, msgRead.data);
                        } else {
                            // [C] NO - Preparo il messaggio di errore
                            msgWrite.status = 404;
                            int written = snprintf(msgWrite.data, sizeof(msgWrite.data), "File %s non trovato", msgRead.data);
                            // Check for truncation and ensure null-termination
                            if (written < 0 || written >= (int)sizeof(msgWrite.data)) {
                                // Truncated, ensure null-termination and optionally log warning
                                msgWrite.data[sizeof(msgWrite.data) - 1] = '\0';
                                // Optionally log: fprintf(stderr, "Warning: msgWrite.data truncated.\n");
                            }
                            printf("<Server> Errore: %s\n", msgWrite.data);
                        }

                        send(&msgWrite);
                    
                    // [C] Invia il ticket al client
                    // [D] Calcolo SHA256 del file su questo figio
                    // [E] Invio il ticket al client
                    
                    break;
                    case 2: 
                        break;
                    default: break;
            }

            // Chiudo il figlio
            printf("[DEBUG] figlio chius0\n");
            exit(0);
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

    exit(0);
}




