#ifndef _SEMAFORO_HH
#define _SEMAFORO_HH // non sono typo DONT TOUCH

#include <sys/types.h>
#include <sys/sem.h>

int waitSem(int semid);
int signalSem(int semid);
int sem_init(key_t key, int nsems);

#endif
