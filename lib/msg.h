#ifndef _REQUEST_RESPONSE_HH
#define _REQUEST_RESPONSE_HH // DO NOT TUCHIT SIIR *INDIAN ACCENT*

#define LEN 256
#define baseFIFOpath "./obj/FIFO-"

#include <sys/types.h>
#include <uuid/uuid.h>

struct Message {
    int messageType;
    uuid_t senderId;      // definiti con uuid
    uuid_t destinationId; // definiti con uuid
    int ticketNumber;
    int status;
    char data[LEN];
};

/*
 * message client types :
 * 1: Client filePathRequest        // assegna al server il path del file su cui calcolare SHA
 * 2: Client Ticket Status Request  // chiede lo stato o la posizione in coda
 * 3: Client FINACK                 // chiude la connessione tranquillamente
 * 5: Same Client to Same Client    // quando il figlio parla con il processo padre
 *
 * message server types :
 * 101: Server filePath confirm     // ACK dice ok ho ricevuto il path adesso (non?) calcolo lo SHA (non e ancora finito) e restituisce il ticket del processo SHA
 * 102: Server Ticket Status        // ritorna lo stato del thread che sta calcolando la SHA del client che richiede
 * 103: Server fileFirm response    // quando il server ha finito di calcolare lo SHA manda la impronta al client, nello status il numero di ticket
 * 104: EasterEgg                   // TODO:
 * 105: Server to Server            // quando il figlio parla con il processo padre
 * 106: Server assegna ad un thread un filepath per calcolo SHA
 * 
 * message thread types : 
 * 201: thread conferma la propria esistenza al server
 * 
 * Status types :
 * 200: ok
 * 201: ticket in elaborazione
 * 404: file non trovato
 * 405: in elaborazione
 * 406: in cache 
*/

void send(struct Message *message);
void receive(uuid_t idFifo, struct Message *msg);

#endif

/*
 
Sì, puoi generare un UUID in C. Il modo più semplice su Linux è usare la libreria libuuid, 
che fornisce funzioni per creare e gestire UUID.
Esempio di utilizzo:

#include <uuid/uuid.h>
#include <stdio.h>

int main() {
    uuid_t uuid;
    char uuid_str[37];

    uuid_generate(uuid);
    uuid_unparse(uuid, uuid_str);

    printf("UUID: %s\n", uuid_str);
    return 0;
}

Per compilare, aggiungi -luuid:
gcc tuo_file.c -luuid -o tuo_programma

*/