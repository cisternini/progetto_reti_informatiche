# 🍽️ Sistema di Gestione Ristorante Distribuito

Questo progetto implementa un'applicazione distribuita basata sul paradigma **Client-Server** per la gestione delle prenotazioni e delle comande di un ristorante. [cite\_start]Il sistema è stato sviluppato in linguaggio C utilizzando socket TCP/IP per il corso di Reti Informatiche (A.A. 2022/2023)[cite: 968, 971].

## 📋 Descrizione del Progetto

Il sistema simula l'infrastruttura informatica di un ristorante, gestendo l'intero flusso di lavoro: dalla prenotazione del tavolo da parte del cliente, all'ordinazione tramite tablet al tavolo, fino alla gestione della preparazione in cucina.

L'architettura prevede un server centrale multiservizio che gestisce le comunicazioni tra tre tipi di client distinti:

1.  **Client Esterno (`cli`)**: Per effettuare prenotazioni.
2.  **Table Device (`td`)**: Tablet situato al tavolo per ordinare e chiedere il conto.
3.  **Kitchen Device (`kd`)**: Dispositivo per la cucina per gestire la coda di preparazione.

## ⚙️ Architettura e Implementazione

Il progetto è sviluppato su sistema Linux e utilizza le socket di Berkeley.

### 1\. Server (`server.c`)

Il server è il cuore del sistema. Gestisce la persistenza dei dati e smista i messaggi tra i vari dispositivi.

  * **Multiplexing I/O**: Utilizza la system call `select()` per gestire contemporaneamente connessioni multiple da client, tablet, dispositivi cucina e input da tastiera (stdin).
  * **Persistenza**: I dati (prenotazioni, menu, comande) sono salvati su file di testo nella directory `./file/`.
  * **Gestione Connessioni**: Identifica il tipo di dispositivo connesso tramite un handshake iniziale ("cli", "td", "kd").

### 2\. Kitchen Device (`kd.c`)

Il dispositivo utilizzato dai cuochi.

  * **Gestione Coda**: Monitora le comande in stato "in attesa" ('a').
  * **Concorrenza**: Quando un `kd` accetta una comanda (`take`), questa viene bloccata per gli altri dispositivi.
  * **Notifiche**: Riceve segnali dal server (es. asterischi `*`) per indicare nuove comande in arrivo.

### 3\. Client & Table Device

  * **Client (`client.c`)**: Permette di interrogare il server sulla disponibilità dei tavoli (`find`) e finalizzare la prenotazione (`book`) ricevendo un codice univoco.
  * **Table Device**: (Implementato logicamente lato server per la gestione socket, il codice lato client gestisce l'interfaccia). [cite\_start]Richiede il codice prenotazione per sbloccarsi[cite: 987].

## 🚀 Compilazione e Esecuzione

### Prerequisiti

  * Compilatore GCC
  * Ambiente Linux/Unix

### Compilazione

È possibile compilare i singoli moduli utilizzando `gcc`:

```bash
gcc -Wall -o server server.c
gcc -Wall -o kd kd.c
gcc -Wall -o client client.c
# Nota: Il Table Device (td) è gestito come entità separata ma condivide logiche simili al client nelle specifiche. 
# Se presente un file td.c non fornito nel contesto, compilarlo similmente.
# Basandosi sui file forniti, le funzionalità td sono gestite lato server o integrate.
```

### Esecuzione

L'ordine di avvio consigliato è:

1.  **Avvio Server:**

    ```bash
    ./server <porta>
    ```

    *Esempio:* `./server 4242`

2.  **Avvio Kitchen Device:**

    ```bash
    ./kd <porta_device>
    ```

    *(Il KD tenterà di connettersi a localhost sulla porta 4242 hardcoded nel codice).*

3.  **Avvio Client (Prenotazioni):**

    ```bash
    ./client <porta_client>
    ```

## 📖 Guida ai Comandi

### Server

  * [cite\_start]`stat`: Mostra lo stato di tutte le comande giornaliere[cite: 1030].
  * [cite\_start]`stat <tavolo>`: Mostra le comande relative a un tavolo specifico (es. `stat T1`)[cite: 1031].
  * [cite\_start]`stat <stato>`: Filtra per stato (`a`=attesa, `p`=preparazione, `s`=servizio)[cite: 1032].
  * [cite\_start]`stop`: Termina il server (solo se non ci sono comande pendenti)[cite: 1054].

### Client (Prenotazione)

  * [cite\_start]`find <cognome> <persone> <data> <ora>`: Cerca disponibilità (es. `find Rossi 4 25-12-23 20`)[cite: 1130].
  * [cite\_start]`book <opzione>`: Prenota un tavolo scelto dalla lista restituita da `find`[cite: 1138].
  * `esc`: Termina il client.

### Kitchen Device (Cucina)

  * [cite\_start]`take`: Prende in carico la comanda più vecchia in attesa[cite: 1101].
  * [cite\_start]`show`: Mostra le comande attualmente in preparazione su questo dispositivo[cite: 1108].
  * [cite\_start]`ready <comanda>-<tavolo>`: Segna una comanda come pronta/in servizio (es. `ready com1-T1`)[cite: 1120].

### Table Device (Tablet)

  * [cite\_start]`menu`: Visualizza i piatti disponibili[cite: 1079].
  * [cite\_start]`comanda <piatto-qta> ...`: Invia un ordine (es. `comanda A1-2 P1-1`)[cite: 1083].
  * [cite\_start]`conto`: Richiede il totale e libera il tavolo[cite: 1087].

## 📂 Struttura dei File

Il sistema richiede la seguente struttura di directory per funzionare correttmente (come desunto da `server.c`):

  * `./server`
  * `./client`
  * `./kd`
  * `./file/` (Directory per i dati persistenti)
      * `tavoli.txt`: Elenco tavoli e configurazione.
      * `prenotazioni.txt`: Storico prenotazioni.
      * `comande.txt`: Log delle comande.
      * `menu.txt`: Listino prezzi.

## ⚠️ Note Tecniche

  * **Protocollo**: I messaggi sono scambiati tramite stringhe formattate o strutture dati serializzate. Il server utilizza `select()` per non bloccare l'esecuzione su un singolo client.
  * [cite\_start]**Validazione**: Lato client (`client.c`) sono implementati controlli rigorosi sul formato data (GG-MM-AA) e ora, oltre che sulla validità logica della prenotazione (es. non prenotare nel passato)[cite: 1131].
  * [cite\_start]**Codici Prenotazione**: Generati randomicamente dal server e associati al timestamp della prenotazione[cite: 983].

-----

*Progetto realizzato seguendo le specifiche del corso di Ingegneria Informatica, Università di Pisa.*
