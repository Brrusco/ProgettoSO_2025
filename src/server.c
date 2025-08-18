#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "errExit.h"

#define LEN 256


int main(int argc, char *argv[]) {
    printf("hello world Server\n");

    char *path2ServerFIFO = "./obj/serverFIFO";
    char *path2ClientFIFO = "./obj/clientFIFO";
    int bufferLettura;
    char clientMessage[LEN];

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
    bufferLettura = read(serverFIFO, &clientMessage[0], sizeof(clientMessage));
    if (bufferLettura == -1) {
        printf("<Server> it looks like the FIFO is broken\n");
    } else if (bufferLettura != sizeof(clientMessage) || bufferLettura == 0)
        printf("<Server> Messaggio ricevuto non va\n");
    else
        printf("<Server> ClientMSG: %s\n", clientMessage);


    // [04] Open the FIFO created by Client in write-only mode
    int clientFIFO = open(path2ClientFIFO, O_WRONLY);
    if (clientFIFO == -1)
        errExit("open failed");

    // [05] Mando al Client la conferma di ricevuta messaggio
    char serverMessage[2*LEN];
    strcat(serverMessage, clientMessage);  // Concatena str2 a str1
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

// Returns 0 on success, or -1 on error
int mkfifo(const char *pathname, mode_t mode);