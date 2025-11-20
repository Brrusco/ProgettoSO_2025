# Todo List Progetto SOP

- [x] Creare Server e Client che si parlano tra di loro con FIFO
- [x] Creare meccanismo di generazione SHA
- [x] Struct comunicazione tra client e server
- [x] Istanzia piu thread server per gestire richieste client multiple in parallelo (limitati)
- [x] Scheduling in ordine di dimensione file (prima il più piccolo variabile)
- [x] Limitare il numero massimo di thread eseguibili server 
- [x] Caching a memoria delle coppie PathFile / ImprontaSHA per restuituzione diretta in caso di richieste uguali ripetute
- [x] Gestire richieste multiple simultanee per un dato percorso processando una sola richiesta ed attendendo il risultato nelle restanti richieste. Se quando sto calcolando una impronta (es path/file1) e ricevo un altra richiesta uguale mentre e ancora in esecuzione aspetto la fine del primo calcolo
- [x] Implementa "thread Pool" : tutti i thread sono sempre in esecuzione ogni volta che finiscono la loro task ne prendono direttamente un altra