#include "errExit.h"

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

void errExit(const char *msg) {
    perror(msg);
    fflush(stdout);
    raise(SIGINT);
}
