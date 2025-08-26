#include "errExit.h"

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

char *path2ServerFIFO = "./obj/serverFIFO";
char *path2ClientFIFO = "./obj/clientFIFO";

void errExit(const char *msg) {
    perror(msg);
    printf("<errExit> removing FIFO...\n");
    if (unlink(path2ServerFIFO) != 0)
        printf("<errExit> unlink failed\n");
    fflush(stdout);
    exit(1);
}
