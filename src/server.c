#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "errExit.h"

#define LEN 256



int main(int argc, char *argv[]) {




    printf("hello world Server\n");

    char *path2ServerFIFO = "/tmp/myfifo";
    if (mkfifo(path2ServerFIFO, S_IRUSR | S_IWUSR | S_IWGRP) == -1)
        errExit("mkfifo failed");

    printf("<Server> FIFO %s created!\n", path2ServerFIFO);

    // Opening for reading only
    int serverFIFO = open(path2ServerFIFO, O_RDONLY);
    if (serverFIFO == -1)
        errExit("open failed");

    char filePath[LEN];
    printf("<Server> waiting for vector [a,b]...\n");
    // Reading 2 integers from the FIFO.
    int bufferLettura = read(serverFIFO, &filePath[0], sizeof(filePath));
    
    // Printing buffer on stdout
    printf("%s\n", bufferLettura);
    
    // closing the fifo
    close(serverFIFO);
    
    // Removing FIFO
    unlink(path2ServerFIFO);


    exit(0);
}

// Returns 0 on success, or -1 on error
int mkfifo(const char *pathname, mode_t mode);