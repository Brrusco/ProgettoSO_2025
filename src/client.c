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

int main(int argc, char *argv[]) {

    printf("hello world - Client\n");

    char clientMessage [LEN];
    struct Message msgRead;
    struct Message msgWrite;
    uuid_t clientId;
    uuid_t serverId;
    char uuid_str[37];

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

    
    // [02] Apro la fifo del srv per scrivere le richieste


    // [05] Attendo il server (02 SRV)


    while(1){
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

        if(fork() == 0){ // apertura figlio in modalita lettura             (ATTENZIONE non sono sicuro fork ==0 sia il filgio potrebbe essere il padre)
            // [06] Lettura della FIFO (03 SRV)
            receive(clientId, &msgRead);
        }
        
    }
   
    if (unlink(path2ClientFIFO) != 0)
        errExit("unlink clientFIFO failed");

    exit(0);
}