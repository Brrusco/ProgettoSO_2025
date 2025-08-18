#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "errExit.h"

#define LEN 256

int main(int argc, char *argv[]) {

    printf("hello world - Client\n");

    char *path2ServerFIFO = "./obj/serverFIFO";
    char *path2ClientFIFO = "./obj/clientFIFO";
    char serverResponse[LEN];
    char clientMessage [LEN];
    int serverFIFO;
    int clientFIFO;
    int bufferLettura;

    printf("<Client> opening FIFO %s...\n", path2ClientFIFO);

    // [01] Creo FIFO per il server x Risposta (01 SRV)
    if (mkfifo(path2ClientFIFO, S_IRUSR | S_IWUSR | S_IWGRP) == -1) {
        if (errno != EEXIST)
            errExit("mkfifo clientFIFO failed");
        else
            printf("<Client> FIFO %s already exists, using it.\n", path2ClientFIFO);
    }
    else
        printf("<Client> FIFO %s created!\n", path2ClientFIFO);

    // [02] Open the FIFO in write-only mode
    serverFIFO = open(path2ServerFIFO, O_WRONLY);
    if (serverFIFO == -1)
        errExit("open serverFIFO failed");

    // [03] Chiedo all'utente la path del file
    printf("<Client> Dammi la Path del File su cui eseguire SHA256: ");
    scanf("%s", clientMessage);

    // [04] Mando al Server il path del file
    printf("<Client> inviando il messaggio : %s\n", clientMessage);
    if (write(serverFIFO, &clientMessage[0], sizeof(clientMessage)) != sizeof(clientMessage))
        errExit("write failed");

    
    // [05] Attendo il server (02 SRV)
    printf("<Client> Aspettando il server...\n");
    clientFIFO = open(path2ClientFIFO, O_RDONLY);
    if (clientFIFO == -1)
        errExit("open clientFIFO failed");
    else
        printf("<Client> Aspettando la risposta del server...\n");
    

    // [06] Lettura della FIFO (03 SRV)
    bufferLettura = read(clientFIFO, &serverResponse[0], sizeof(serverResponse));
    if (bufferLettura == -1) {
        printf("<Client> it looks like the FIFO is broken\n");
    } else if (bufferLettura != sizeof(serverResponse) || bufferLettura == 0)
        printf("<Client> Risposta ricevuta non valido\n");
    else
        printf("<Client> ServerMSG: %s\n", serverResponse);
        



    // [c0] SRV Chiude la FIFO del server (l'Unlink lo fa  il server oppure errExit)
    if (close(serverFIFO) != 0)
        errExit("close serverFIFO failed");

    // [c1] Chiudo e rimuovo la FIFO del client
    if (close(clientFIFO) != 0)
        errExit("close clientFIFO failed");
    if (unlink(path2ClientFIFO) != 0)
        errExit("unlink clientFIFO failed");

    exit(0);
}