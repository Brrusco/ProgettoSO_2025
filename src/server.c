#include <stdio.h>
#include <stdlib.h>

int main() {

    printf("hello world Server\n");

    char *fname = "/tmp/myfifo";
    int res = mkfifo(fname, S_IRUSR|S_IWUSR);

    // Opening for reading only
    int fd = open(fname, O_RDONLY);
    
    // reading bytes from fifo
    char buffer[LEN];
    read(fd, buffer, LEN);
    
    // Printing buffer on stdout
    printf("%s\n", buffer);
    
    // closing the fifo
    close(fd);
    
    // Removing FIFO
    unlink(fname);


    exit(0);
}