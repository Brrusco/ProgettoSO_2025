#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "errExit.h"

int main(int argc, char *argv[]) {

    printf("hello world Client\n");

    char *path2ServerFIFO = "/tmp/myfifo";

    printf("<Client> opening FIFO %s...\n", path2ServerFIFO);
    // Open the FIFO in write-only mode
    int serverFIFO = open(path2ServerFIFO, O_WRONLY);
    if (serverFIFO == -1)
        errExit("open failed");


    // Close the FIFO
    if (close(serverFIFO) != 0)
        errExit("close failed");
    
    exit(0);
}