#ifndef _THREADOP_HH
#define _THREADOP_HH // non sono typo DONT TOUCH

void *threadOp(void *arg);// void *arg permette di passare 0 o piu argomenti alla funzione (poi da controllare in codice) 

struct ThreadData {
    uuid_t threadId; // condiviso tra tutti i threa d, e l'id della fifo su cui i thread fanno la receive , va definito dal server prima della creazione del thread
};

#endif
