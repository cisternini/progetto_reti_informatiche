#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>

struct cmd_ing{ // struttura per la comandi
    char cmd[5];
    char par[3];
};

typedef enum{false,true}bool;

struct tavoli{ // struttura per gestire i tavoli
    int opz;
    char t_nome[4]; // nome del tavolo
    char posizione[6]; // posizione del tavolo
    int pers; // numero di persone
    char descrizione[20];
    struct tavoli* succ;
};

struct righe{ // strutture per la gestione delle righe delle comande
    char codice[6];
    int quantita;
    struct righe* succ;
};

struct comanda{ // struttura per la gestione delle comande
    char c_comanda[5];
    char c_tavolo[4];
    char stato;
    int n_piatti;
    struct righe* piatti;
};

struct td { // struttura per la  gestione dei tablet
    int socket; // salva il socket dei tablet
    bool approved; // variabile per controllo dell'accesso
    int numero_comanda; // conta le comande richieste dal tablet
    char tavolo[4]; // crea la corrispondeza fra tablet e tavolo
    struct td* succ;
};


struct clienti{ // struttura per la gestione dei clienti
    char nome[30]; 
    int num_pers; 
    char data[9];
    char ora[3];
    int socket; // salva il socket dei clienti
    struct clienti* succ;
    struct tavoli* tavoli_disp; // salva i tavoli disponibili per il cliente
};

struct kd{
    int socket; // socket dei kitchen
    struct kd* succ;
};

#define ID_LEN 3
#define BUFFER_LEN 1024
#define LEN_RIGHE 8
#define LEN_COD 5

void stampa_comande_tavolo(char* tavolo); // funzione per la stampa dei comande di un tavolo
void stampa_comande_stato(char stato); // funzione che stampa le comande nello stato richiesto
bool controlla_comande(); // controlla le comande attive
struct clienti* aggiungi_cliente(struct clienti*,int); // aggiunge il cliente appena connesso alla lista cli
struct td* aggiungi_td(struct td*,int); // ----
struct kd* aggiungi_kd(struct kd*,int); // ----
struct clienti* trova_socket_cliente(int,struct clienti*); // trova il cliente corrispondete al socket
struct clienti* trova_tavoli(struct clienti*,uint16_t*); // trova i tavoli disponibili
bool compara_data(char*,char*); // compara due date
bool compara_ora(char*,char*); // compara  due ore
struct tavoli* elimina_dalla_lista(struct tavoli*,struct tavoli*); // elimina dalla lista dei tablet il tavolo selezionato
struct tavoli* tavolo_prenotato(struct tavoli*,int); // controlla il tavolo prenotato dal cliente
void genera_codice(char*); // genera il codice
struct clienti* azzera_struttura_cliente(struct clienti*); // svuota la strutta dei clienti
struct td* trova_socket_tavolo(int,struct td*);// trova il tablet corrispondente al socket
bool controllo_prenotazione(char*,struct td*); // controllo il codice della prenotazione
bool check_cmd(struct cmd_ing); // controlla il formato dei comandi
struct clienti* rimuovi_clienti(int,struct clienti*); // elimina il client disconesso
int pows(int,int); 
struct kd* rimuovi_kd(int ,struct kd*); // elimina il kd disconnesso
bool controllo_tavolo(char*,struct td*,int); 
struct td* rimuovi_td(int ,struct td*); // elimina il tablet disconnesso




int main(int argc, char* argv[]){
    int ret,i,len;
    fd_set master,read_fds,write_fds,cli_fds,td_fds,kd_fds;

    in_port_t porta = htons(atoi(argv[1]));

    int fdmax;
    struct clienti* cli= NULL; // lista dei client connessi
    struct kd* cuochi = NULL; // lista dei cuochi connessi
    struct td* tablet = NULL; // lista dei tavoli attivi
    struct sockaddr_in sv_addr,cl_addr;
    time_t rawtime; // salvo il timestamp in  tempo reale
    struct tm * timeinfo; // variabile d'appoggio
    int ordini_inattesa = 0; /// conta gli ordini in attessa di preparazione
    
    char buffer[BUFFER_LEN]; // buffer di comunicazione
    char ok[ID_LEN]; // conferma 
    char MessaggioIniziale[1024] = "**************BENVENUTO*****************\n 1) stat tavolo|stato \t\t --> mostra le comande di un tavolo o le comande in uno specifico stato\n 2) stop\t\t\t --> termina il server\n\n";


    struct cmd_ing com;

    int listener;
    int newfd;


    strcpy(ok,"ok");

    listener = socket(AF_INET,SOCK_STREAM|SOCK_NONBLOCK,0);


    memset(&sv_addr,0,sizeof(sv_addr));
    sv_addr.sin_family = AF_INET;
    sv_addr.sin_port = porta;
    sv_addr.sin_addr.s_addr = INADDR_ANY;

    ret = bind(listener,(struct sockaddr*)& sv_addr,sizeof(sv_addr));

    if(ret<0){
        perror("Errore sulla bind:");
        printf("\nArresto in corso...\n");
        exit(1);
    }
    
    ret = listen(listener,10);

    if(ret<0){
        perror("Errore sulla listen:");
        printf("\nArresto in corso...\n");
        exit(1);
    }

    FD_ZERO(&master);
    FD_ZERO(&read_fds);
    FD_ZERO(&cli_fds);
    FD_ZERO(&td_fds);
    FD_ZERO(&kd_fds);

    FD_SET(listener,&master);
    FD_SET(0,&master);

    fdmax = listener;

    

    printf("%s",MessaggioIniziale);

    while(1){

        printf("Premi Enter per inserire un comando....\n");
        read_fds = master;

        ret = select(fdmax+1,&read_fds,NULL,NULL,NULL);

        if(ret<0){
            perror("Errore Select:");
            exit(1);
        }

        for(i = 0;i<=fdmax;i++){
           

            if(FD_ISSET(i,&read_fds)){

                    if(i==0){
                        do{
                            printf("%s","Digita un comando: ");
                            scanf("%s",com.cmd);

                            if(strcmp(com.cmd,"stat")==0){
                                scanf("%s",com.par);
                            }

                        }while(check_cmd(com)==false);

                        if(strcmp(com.cmd,"stat")==0){
                            switch (com.par[0]){
                                case 'T':
                                    stampa_comande_tavolo(com.par);
                                    break;
                                case 'a': case 'p': case 's':
                                    stampa_comande_stato(com.par[0]);
                                    break;
                            }
                        }else{
                            bool controllo =false;
                            int j;
                            int len;
                            char cl[2];
                            strcpy(cl,"cl");
                            controllo=controlla_comande(); // controllo le comande attive
                            if(controllo==true){
                                for(j=0;j<= fdmax;j++){
                                    if(FD_ISSET(j,&kd_fds)||FD_ISSET(j,&td_fds)){ // invio segnale di spegnimento 
                                        len = strlen(cl);
                                        ret = send(j,(void*)cl,len,0);
                                    }
                                    
                                }
                                close(listener);
                                remove("./file/comande.txt"); // eliminazione comande odierne
                                exit(1);

                            }else{
                                printf("Ci sono ancora comande in attesa o in preparazione. Attendere...\n");
                            }
                           
                        }



                    }else if(i==listener){
                                char id[3];
                                printf("Nuovo client rilevato!\n");
                                fflush(stdout);
                                len = sizeof(cl_addr);
                                newfd = accept(listener,(struct sockaddr*)&cl_addr,&len);

                                if(newfd<0){
                                    perror("Errore sull'accept:");
                                    printf("\nArresto in corso\n");
                                    continue;
                                }
                                FD_SET(newfd,&master);
                               
                                if(newfd>fdmax){fdmax = newfd;}

                                ret = recv(newfd,(void*)id,ID_LEN,0);

                                if(ret<0){
                                    perror("Errore lettura id: ");
                                    exit(1);
                                }

                                printf("Aggiunta nuovo client alla lista...\n");
                                fflush(stdout);

                               
                               

                                if(strncmp(id,"cli",3)==0){ // controllo di che tipo di device si tratta
                                    FD_SET(newfd,&cli_fds);
                                    cli = aggiungi_cliente(cli,newfd);
                                } else if(strcmp(id,"td")==0){
                                    FD_SET(newfd,&td_fds);
                                    tablet = aggiungi_td(tablet,newfd);
                                } else if(strncmp(id,"kd",2)==0){
                                    char cl[2];
                                    FD_SET(newfd,&kd_fds);
                                    cuochi = aggiungi_kd(cuochi,newfd);
                                    if(ordini_inattesa>0){// aggiorno il kd appena collegato su quanti ordini ci sono in attesa
                                        sprintf(cl,"%i",ordini_inattesa);
                                      
                                        ret = send(newfd,(void*)cl,2,0);
                                        
                                    }
                                }

                    }else{
                                if(FD_ISSET(i,&cli_fds)){
                                            char comando[4];
                                            ret = recv(i,(void*)buffer,BUFFER_LEN,0);
                                    
                                            if(ret<0){
                                                printf("Nessun comando inserito...\n");
                                                break;
                                                
                                            }
                                            if(ret == 0){ // disconnessione in caso chiusura del cliente
                                                printf("Disconnessione Cliente!!\n");
                                                FD_CLR(i,&master); // modifico i fd_set
                                                FD_CLR(i,&cli_fds);
                                                if(i==fdmax)
                                                    fdmax = fdmax-1;
                                                cli = rimuovi_clienti(i,cli); // elimina dalla lista dei cliente
                                                continue;
                                            }

                                            printf("Ricezione comando...\n");
                                            fflush(stdout);


                                            if(strncmp(buffer,"find",4)==0){
                                                        struct clienti* attivo = trova_socket_cliente(i,cli);
                                                        struct tavoli* da_inviare;
                                                        int prova = 0;
                                                        uint16_t n_tavoli;
                                                       
                                                        sscanf(buffer,"%s %s %i %s %s",comando,attivo->nome,&attivo->num_pers,attivo->data,attivo->ora); // parse del comando
                                                       
                                                        printf("Ricerca dei tavoli disponibili...\n");
                                                        attivo = trova_tavoli(attivo,&n_tavoli);  // cerca i tavoli disponibili
                                                        
                                                        printf("Invio del numero dei tavoli...\n");
                                                        n_tavoli = htons(n_tavoli);

                                                        ret = send(i,(void*)&n_tavoli,sizeof(uint16_t),0);

                                                        if(ret<0){
                                                                    perror("Errore in fase di invio: \n");
                                                                    exit(-1);
                                                            }

                                                        printf("Invio elenco dei tavoli disponibili...\n");
                                                        da_inviare = attivo->tavoli_disp;

                                                        while(da_inviare != NULL){ // invio e serializzo i tavoli e le loro descrizioni

                                                            sprintf(buffer,"%i %s %s %s",da_inviare->opz,da_inviare->t_nome,da_inviare->posizione,da_inviare->descrizione);

                                                            ret = send(i,(void*)buffer,BUFFER_LEN,0);
                                                            
                                                            if(ret<0){
                                                                    perror("Errore in fase di invio: \n");
                                                                    exit(-1);
                                                            }


                                                            da_inviare = da_inviare->succ;

                                                        }

                                                     }else{
                                                        struct clienti* attivo = trova_socket_cliente(i,cli); // cerco il client collegato al socket
                                                        struct tavoli* prenotato;
                                                        int opz;
                                                        char codice[6];
                                                        printf("Ricezione scelta del cliente...\n");
                                                    

                                                        sscanf(buffer,"%s %i",comando,&opz); // parse del comando book
                                                        
                                                        prenotato = tavolo_prenotato(attivo->tavoli_disp,opz);
                                                        
                                                        time(&rawtime);

                                                        timeinfo = localtime(&rawtime);

                                                        genera_codice(codice);

                                                        printf("Salvataggio prenotazione...\n"); // salvo la prenotazione con codice e timestamp
                                                        FILE* prenotazioni = fopen("./file/prenotazioni.txt","a+");

                                                        fprintf(prenotazioni,"%s %s %s %s %s %i %d:%d:%d %d-%d-%d\n",codice,prenotato->t_nome,attivo->ora,attivo->data,attivo->nome,attivo->num_pers,timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec,timeinfo->tm_mday,timeinfo->tm_mon+1,timeinfo->tm_year+1900);

                                                        fclose(prenotazioni);

                                                        printf("Invio conferma e codice ...\n"); 

                                                        codice[5] = '\0'; 

                                                        sprintf(buffer,"%s %s",ok,codice);   

                                                
                                                        ret = send(i,(void*)buffer,BUFFER_LEN,0); // invio conferma e codice

                                                        if(ret<0){
                                                            perror("Errore invio ok:");
                                                            continue;
                                                        }

                                                       attivo = azzera_struttura_cliente(attivo); // svuoto la struttra attivo per poter permette ad altri client di cercare
                                                
                                                }

                                    }else if(FD_ISSET(i,&td_fds)){
                                            struct td* attivo = trova_socket_tavolo(i,tablet);  // cerco il tablet collegato al socket attivo
                                            char ing_codice[5]; // prelevo il codice inserito dall'utente
                                            char comando[10]; // prelevo il comando scritto dall'utente
                                            

                                            if(attivo->approved == false){   
                                                                                        
                                                        ret = recv(i,(void*)ing_codice,LEN_COD,0);

                                                       
                                                        if(ret == 0){
                                                            printf("Disconnessione Tablet:%i\n",i);
                                                            FD_CLR(i,&master);
                                                            FD_CLR(i,&td_fds);
                                                            if(i==fdmax)
                                                                fdmax = fdmax-1;
                                                            tablet = rimuovi_td(i,tablet);
                                                            continue;
                                                        }


                                                      
                                                        if(ret<0){
                                                            perror("Errore nella ricezione del codice:");
                                                            break;
                                                        }
                                                     
                                                       
                                                        printf("Ricezione codice del cliente...\n");

                                                        attivo->approved = controllo_prenotazione(ing_codice,attivo); // controlla che il codice corrisponda ad una prenotazione e che data e ora siano corretti

                                                        attivo->approved = controllo_tavolo(attivo->tavolo,tablet,i); // controlla che il codice non sia di un tavolo gia attivo
                                                       
                                                        if(attivo->approved == true){
                                                                printf("Codice trovato...\n");
                                                                printf("Invio conferma...\n");
                                                                char ok[2] = "ok";
                                                                ret = send(i,(void*)ok,2,0);     
                                                        } else {
                                                            printf("Codice non trovato ...\n");
                                                            printf("Invio segnale ...\n");
                                                            strcpy(attivo->tavolo," ");
                                                            char ok[2] = "ko";
                                                            ret = send(i,(void*)ok,2,0);
                                                            
                                                        }

                                                                                                                                                           
                                                   
                                             }
                                             else {
                                                uint16_t len_com;
                                                uint16_t n_piatti;
                                                int j;
                                                FILE* comande = fopen("./file/comande.txt","a+");

                                               
                                                ret = recv(i,(void*)&len_com,sizeof(uint16_t),0);

                                                if(ret == 0){
                                                    printf("Disconnessione Tablet:%i\n",i);
                                                    FD_CLR(i,&master);
                                                    FD_CLR(i,&td_fds);
                                                    if(i==fdmax)
                                                        fdmax = fdmax-1;
                                                    tablet = rimuovi_td(i,tablet);
                                                    continue;
                                                }

                                                len = ntohs(len_com);

                                                

                                                ret = recv(i,(void*)comando,len,0);

                                                

                                                if(ret < 0){
                                                    perror("Errore nella recv comando:");
                                                    continue;
                                                   
                                                }

                                              
                                                if(strcmp(comando,"comanda")==0){
                                                        struct comanda* nuova_comanda = (struct comanda*) malloc(sizeof(struct comanda));
                                                        uint16_t l_piatto; // salva la lunghezza del codice del piatto inviato
                                                        // variabili di appoggio
                                                        char piatto[4]; 
                                                        char tipo;
                                                        char cl[2];
                                                        char ordine[2];


                                                        printf("Ricezione numero di piatti...\n");                            
                                                        ret = recv(i,(void*)&n_piatti,sizeof(uint16_t),0);

                                                        if(ret < 0){
                                                            perror("Errore nella recv n_piatti:");
                                                            break;
                                                        }        
                                                    
                                                        n_piatti = ntohs(n_piatti);

                                                        attivo->numero_comanda++;

                                                        // creazione del titotl della comanda

                                                        strcpy(nuova_comanda->c_comanda,"com");

                                                        sprintf(ordine,"%i",attivo->numero_comanda);

                                                        strcat(nuova_comanda->c_comanda,ordine);

                                                        // salvo il numero di piatti della comanda per poter vettorizzare successivamente
                                                        nuova_comanda->n_piatti = n_piatti;
                                                        printf("Individuazione del tavolo...\n");                      
                                                        strcpy(nuova_comanda->c_tavolo,attivo->tavolo);
                                                        // creo un vettore per i piatti
                                                        nuova_comanda->piatti = (struct righe*)malloc(sizeof(struct righe) * n_piatti);
                                                        printf("Ricezione comande e salvataggio...\n");  
                                                        for(j = 0; j<n_piatti; j++){ // salva man mano i piatti nel vettore

                                                                ret = recv(i,(void*)&l_piatto,sizeof(uint16_t),0);


                                                                if(ret < 0){
                                                                    perror("Errore nella recv l_piatto:");
                                                                    printf("\n");
                                                                    break;
                                                                }  

                                                                l_piatto = ntohs(l_piatto);

                                                                ret = recv(i,(void*)piatto,l_piatto,0);

                                                                  if(ret < 0){
                                                                    perror("Errore nella recv l_piatto:");
                                                                    printf("\n");
                                                                    break;
                                                                }  


                                                                sscanf(piatto,"%s",nuova_comanda->piatti[j].codice);


                                                        }
                                                        printf("Scrittura nel database...\n");
                                                        ordini_inattesa++;
                                                        // creo la riga della comanda
                                                        tipo = 'c';

                                                        nuova_comanda->stato = 'a';
                                                        // la salvo nel file
                                                        fprintf(comande,"%c %s %s %c %i\n",tipo,nuova_comanda->c_comanda,nuova_comanda->c_tavolo,nuova_comanda->stato,nuova_comanda->n_piatti);
                                                        
                                                        tipo = 's';

                                                        

                                                        for(j = 0;j<n_piatti;j++){ // salvo i piatti nel file
                                                                int esp;
                                                                nuova_comanda->piatti[j].quantita = 0;
                                                               
                                                                len = strlen(nuova_comanda->piatti[j].codice);
                                                               
                                                                for(esp = 3;esp<len;esp++){  // uso questo metodo per eventuali  quantita a piu cifre
                                                                   nuova_comanda->piatti[j].quantita += ( nuova_comanda->piatti[j].codice[esp]-48) * pows(10,len - esp); // modificare per rendere il numero su piu cifre
                                                                   nuova_comanda->piatti[j].codice[esp]= '\0';
                                                                }
                                                                fprintf(comande,"%c %c%c %i\n",tipo,nuova_comanda->piatti[j].codice[0],nuova_comanda->piatti[j].codice[1], nuova_comanda->piatti[j].quantita);                                                            
                                                        }

                                                        sprintf(cl,"%i",ordini_inattesa);
                                                        printf("Invio segnale ai kitchen device...\n");
                                                        for(j = 0;j<=fdmax;j++){ // aggiorno il kd sul numero di ordini in attesa
                                                             
                                                            if(FD_ISSET(j,&kd_fds)){
                                                                len = 2;
                                                                ret = send(j,(void*)cl,len,0);
                                                                  if(ret < 0){
                                                                    perror("Errore nella send ai kd:");
                                                                    printf("\n");
                                                                    continue;
                                                                }  
                                                            }
                                                        }

                                                        free(nuova_comanda->piatti);
                                                        free(nuova_comanda);

                                                        fclose(comande);
                                                    
                                                }else if(strncmp(comando,"conto",5)==0){ 
                                                            struct comanda* ordini_effettuati = (struct comanda*) malloc( sizeof(struct comanda) * attivo->numero_comanda);
                                                            // variabili d'appoggio
                                                            char tipo;
                                                            char codice_piatto[2];
                                                            int j,k;
                                                            char riga_conto[LEN_RIGHE];
                                                            uint16_t tot_piatti=0; // conta il numero di piatti ordinati dal tavolo

                                                            if(attivo->numero_comanda==0){
                                                                printf("Non ci sono comande per questo tavolo.....\n");
                                                                tot_piatti = htons(tot_piatti);
                                                                ret = send(i,(void*)&tot_piatti,sizeof(uint16_t),0);
                                                                fclose(comande);
                                                                break;
                                                            }

                                                        

                                                            for(j=0;j<attivo->numero_comanda;j++){ // preleva le comande e controlla se  appartendogno al tavolo, se si salva i piatti altrimeti scorre i lfile
                                                                             
                                                                                    fscanf(comande,"%*c %s %s %c %i\n",ordini_effettuati[j].c_comanda,ordini_effettuati[j].c_tavolo,&ordini_effettuati[j].stato,&ordini_effettuati[j].n_piatti);
                                                                                    if(strcmp(ordini_effettuati[j].c_tavolo,attivo->tavolo)==0){
                                                                                                struct righe* piatti_tmp;
                                                                                                ordini_effettuati[j].piatti = (struct righe*) malloc(sizeof(struct righe)* ordini_effettuati[j].n_piatti);
                                                                                                piatti_tmp = ordini_effettuati[j].piatti;
                                                                                                for(k = 0;k<ordini_effettuati[j].n_piatti;k++){
                                                                                                    fscanf(comande,"%*c %c%c %i\n",&piatti_tmp[k].codice[0],&piatti_tmp[k].codice[1],&piatti_tmp[k].quantita);

                                                                                                }                                                                            
                                                                                    } else {
                                                                                        for(k = 0;k<ordini_effettuati[j].n_piatti;k++){
                                                                                                    fscanf(comande,"%*c %*c%*c %*i\n");
                                                                                                } 
                                                                                        j--;
                                                                                    }
                                                                             
                                                            }
                                                            
                                                            for(j=0;j<attivo->numero_comanda;j++){ // conta i piatti
                                                                tot_piatti += ordini_effettuati[j].n_piatti;
                                                            }


                                                            tot_piatti = htons(tot_piatti);

                                                            ret = send(i,(void*)&tot_piatti,sizeof(uint16_t),0);
                                                            printf("Invio numero di piatti totali...\n");
                                                            printf("Invio del costo di ogni piatto ordinato...\n");
                                                            for(j=0;j<attivo->numero_comanda;j++){ // invia comanda per comanda i piatti e i loro costo
                                                                struct righe* piatti_tmp;
                                                                int costo_singolo;
                                                                piatti_tmp = ordini_effettuati[j].piatti;
                                                                
                                                                for(k = 0;k<ordini_effettuati[j].n_piatti;k++){
                                                                        FILE* menu = fopen("./file/menu.txt","r+");
                                                                        while(!feof(menu)){
                                                                            fscanf(menu,"%s %i\n",codice_piatto,&costo_singolo);
                                                                           
                                                                            if(strncmp(codice_piatto,piatti_tmp[k].codice,2)==0){
                                                                                break;
                                                                            }                                                              
                                                                        }
                                                                         printf("%s %i %s %i\n",codice_piatto,costo_singolo,piatti_tmp[k].codice,costo_singolo*piatti_tmp[k].quantita);
                                                                        sprintf(riga_conto,"%c%c %i %i",piatti_tmp[k].codice[0],piatti_tmp[k].codice[1],piatti_tmp[k].quantita,costo_singolo*piatti_tmp[k].quantita);
                                                                        
                                                                        ret = send(i,(void*)riga_conto,LEN_RIGHE,0);

                                                                        fclose(menu);
                                                                }   
                                                            }

                                                        fclose(comande);

                                                }


                                             }

                                        


                                    } else if(FD_ISSET(i,&kd_fds)){
                                        char prenotato[2];
                                        char cl[2];

                                       
                                        ret = recv(i,(void*)prenotato,2,0);
                                      
                                        if(strncmp(prenotato,"ok",2)==0){ //riceve un avviso nel caso in cui un kd abbia prelevato la prima comanda in  attesa
                                            int j;
                                            ordini_inattesa--;
                                            printf("Un kitchen device ha accettato l'ultima comanda immessa...\n");
                                            sprintf(cl,"%i",ordini_inattesa);
                                            for(j=0;j<=fdmax;j++){
                                                if(FD_ISSET(j,&kd_fds)){
                                                    ret = send(j,(void*)cl,2,0);
                                                }
                                            }
                                           
                                            strcpy(prenotato," ");
                                        }
                                        else if(ret==0){ // avverte e elimina il kd che si e' chiuso involontariamente
                                            printf("Disconnessione Kitchen:%i\n",i);
                                            FD_CLR(i,&master);
                                            FD_CLR(i,&kd_fds);
                                            cuochi = rimuovi_kd(i,cuochi);
                                            if(i==fdmax)
                                                    fdmax = fdmax-1;
                                             continue;
                                        }

                                    }

                    }


            }


        }
    }

    return 0;

}





/**************************************************FUNZIONI**********************************************************/


void stampa_comande_tavolo(char* tavolo){
    FILE* comande = fopen("./file/comande.txt","r+");
    char tipo;
    int i;
    struct comanda* com = (struct comanda*)malloc(sizeof(struct comanda));
    struct righe* piatto = (struct righe*)malloc(sizeof(struct righe)); 
    char *stato;

    if(comande == NULL){
        printf("Non ci sono comande");
        return;
    }

    while(!feof(comande)){
             fscanf(comande,"%c %s %s %c %i\n",&tipo,com->c_comanda,com->c_tavolo,&com->stato,&com->n_piatti);
             if(tipo == 'c' && strcmp(tavolo,com->c_tavolo)==0){
                    if(com->stato == 'p'){ 
                        stato = "<in preparazione>";
                    } else if(com->stato == 'a'){
                        stato = "<in attesa>";
         
                    }else{
                        stato = "<in servizio>";
                    }

                    printf("%s %s\n",com->c_comanda,stato);
                    for(i = 0;i<com->n_piatti;i++){
                                fscanf(comande,"%*c %s %i\n",piatto->codice,&piatto->quantita);
                                printf("%s %i\n",piatto->codice,piatto->quantita);
                    }


             }
    }

free(com);
free(piatto);

fclose(comande);
return;

}


void stampa_comande_stato(char stato){
    FILE* comande = fopen("./file/comande.txt","r+");
        char tipo;
    int i;
    struct comanda* com = (struct comanda*)malloc(sizeof(struct comanda));
    struct righe* piatto = (struct righe*)malloc(sizeof(struct righe)); 
    if(comande==NULL){
        printf("Non ci sono comande\n");
        return;    
        }
    while(!feof(comande)){
             fscanf(comande,"%c %s %s %c %i\n",&tipo,com->c_comanda,com->c_tavolo,&com->stato,&com->n_piatti);
             if(tipo == 'c' && stato==com->stato){

                    printf("%s %s\n",com->c_comanda,com->c_tavolo);
                    for(i = 0;i<com->n_piatti;i++){
                                fscanf(comande,"%*c %s %i\n",piatto->codice,&piatto->quantita);
                                printf("%s %i\n",piatto->codice,piatto->quantita);
                    }


             }
    }

    free(com);
    free(piatto);
    fclose(comande);
    return;
}

bool controlla_comande(){
    FILE* comande = fopen("./file/comande.txt","r+");
    struct comanda* com = (struct comanda*)malloc(sizeof(struct comanda));
    char tipo;
    if(comande==NULL){
        printf("Nessuna comanda attiva\n");
        return true;
    }
    while(!feof(comande)){
              fscanf(comande,"%c %s %s %c %i",&tipo,com->c_comanda,com->c_tavolo,&com->stato,&com->n_piatti);
              if(tipo=='c' && (com->stato == 'a' || com->stato =='p')){
                return false;
              }

    }
    fclose(comande);
    return true;
}


struct clienti* aggiungi_cliente(struct clienti*cli,int id){
            struct clienti* new =  (struct clienti*)malloc(sizeof(struct clienti));            
            struct clienti* tmp= cli;
            new->socket = id;
            new->succ =NULL;

           

            if(tmp ==NULL){
                cli = new;
            }else {
                while(tmp->succ !=NULL)
                    tmp = tmp->succ;

                tmp->succ = new;

            }

            return cli;
}


struct td* aggiungi_td(struct td* td,int id){
            struct td* new =  (struct td*)malloc(sizeof(struct td));
         
            struct td* tmp=td;
            new->socket = id;
            new->approved = false;
            new->numero_comanda = 0;
            new->succ =NULL;
            if(tmp ==NULL){
                td = new;
            }else {
                while(tmp->succ !=NULL)
                    tmp = tmp->succ;

                tmp->succ = new;

            }

            return td;
}


struct kd* aggiungi_kd(struct kd*kd,int id){
     struct kd* new =  (struct kd*)malloc(sizeof(struct kd));
         
            struct kd* tmp=kd;
            new->socket = id;
            new->succ =NULL;
            if(tmp ==NULL){
                kd = new;
            }else {
                while(tmp->succ !=NULL)
                    tmp = tmp->succ;

                tmp->succ = new;

            }

        return kd;
}

struct clienti* trova_socket_cliente(int id,struct clienti* cli){
    struct clienti* tmp = cli;

    while(tmp->socket != id && tmp->succ != NULL){
        tmp = tmp->succ;
    }
     if(tmp->socket == id){
         return tmp;  
        }
      else return NULL;
    
}


struct clienti* trova_tavoli(struct clienti* attivo,uint16_t* num){
    
    uint16_t num_tavoli=0;
    struct tavoli* tmp;
    FILE* tavoli = fopen("./file/tavoli.txt","r+");
   
    if(tavoli==NULL){
        perror("Errore file: ");
    }
    
    attivo->tavoli_disp = NULL;
    tmp = attivo->tavoli_disp;
   
  
    while(!feof(tavoli)){ // confronto i tavoli che soddisfano il numero di clienti
        struct tavoli* new = (struct tavoli*)malloc(sizeof(struct tavoli));
        new->succ = NULL;
        fscanf(tavoli,"%s %s %i %s",new->t_nome,new->posizione,&new->pers,new->descrizione);
             
        if(new->pers<attivo->num_pers || new->pers>attivo->num_pers+3){
           free(new);
        }
        else{
            if(tmp == NULL){
              
                attivo->tavoli_disp = new;
                
               }
            else{
                
                while(tmp->succ!=NULL)
                tmp = tmp->succ;
                tmp->succ = new;
            }
        }
        tmp = attivo->tavoli_disp;
    }
    fclose(tavoli);

    
    tmp = attivo->tavoli_disp;


 

    while(tmp!=NULL){ // filtro i tavoli che hanno gia' prenotazioni per quell'orario
        
        FILE* prenotazioni = fopen("./file/prenotazioni.txt","r+");
        char tavolo[4];
        char ora[3];
        char data[9];

        if(prenotazioni == NULL){
            perror("Errore apertura file:");
            break;
        }
        
        while(!feof(prenotazioni)){
                    
                   fscanf(prenotazioni,"%*s %s %s %s %*s %*i %*s %*s\n",tavolo,ora,data);
                   if(strcmp(tavolo,tmp->t_nome)==0){  
                            if(compara_data(data,attivo->data)==true && compara_ora(ora,attivo->ora)==true){
                                attivo->tavoli_disp = elimina_dalla_lista(attivo->tavoli_disp,tmp);
                            }
                   }
        }
        fclose(prenotazioni);
        tmp = tmp->succ;
    }

    tmp = attivo->tavoli_disp;

    while(tmp != NULL){
        num_tavoli++;
        tmp->opz = num_tavoli;
        tmp = tmp->succ;
    }


    *num = num_tavoli;
    return attivo;


}


bool compara_data(char* data1,char* data2){
    struct tm t_data1;
    struct tm t_data2;
    
    time_t d1,d2;
    double sec;
    memset(&t_data1,0,sizeof(struct tm));
    memset(&t_data2,0,sizeof(struct tm));
       

    strptime(data1,"%d-%m-%Y",&t_data1);
    strptime(data2,"%d-%m-%Y",&t_data2);

    d1 = mktime(&t_data1);
    d2 = mktime(&t_data2);
    sec = difftime(d2,d1);
    if(sec == 0){
        return true;
    }
       
    else 
        return false;
}


bool compara_ora(char*ora1,char*ora2){
     struct tm t_ora1;
    struct tm t_ora2;
        time_t o1,o2;
    double sec;
    int h;
    int _stoh=3600;

    memset(&t_ora1,0,sizeof(struct tm));
    memset(&t_ora2,0,sizeof(struct tm));

    strptime(ora1,"%H",&t_ora1);
    strptime(ora2,"%H",&t_ora2);

    o1 = mktime(&t_ora1);
    o2 = mktime(&t_ora2);
    sec = difftime(o2,o1);
    h = sec/_stoh;
    if(h>=2 || h<=-2){
        return false;
    }else{
        return true;
    }

}

struct tavoli* elimina_dalla_lista(struct tavoli*testa,struct tavoli*da_eliminare){

    struct tavoli* tmp = testa;
    struct tavoli* elimina = testa->succ;
 
    if(testa->t_nome == da_eliminare->t_nome){
        elimina = testa;
        testa = testa->succ;
        
    }else{
        while(elimina->t_nome != da_eliminare->t_nome){
            tmp = elimina;
            elimina = elimina->succ;
        }

        tmp->succ = elimina->succ;
        elimina->succ = NULL;
    }
    free(elimina);
    return testa;

}



struct tavoli* tavolo_prenotato(struct tavoli* tavolo,int opz){
    struct tavoli* tmp = tavolo;

    while(tmp->opz != opz){
        tmp = tmp->succ;
    }

    return tmp;
}


void genera_codice(char* codice){
    int cifra;
    int i;
    char c;

    srand(time(NULL));

    for(i = 0;i<5;i++){
        cifra = 0 + rand()%10;
        c = cifra + 48;
        codice[i]=c;
    }

}

struct clienti* azzera_struttura_cliente(struct clienti* attivo){
    struct tavoli* tmp = attivo->tavoli_disp;
    struct tavoli* elimina = attivo->tavoli_disp;

    attivo->tavoli_disp = NULL;
    strcpy(attivo->nome,"");
    strcpy(attivo->data,"");
    attivo->num_pers = 0;
    strcpy(attivo->ora,"");
    while(tmp != NULL){
        tmp = tmp->succ;
        free(elimina);
        elimina = tmp;
    }

    return attivo;

}


struct td* trova_socket_tavolo(int id,struct td* tablet){
    struct td* tmp = tablet;
    while(tmp!=NULL){
        if(tmp->socket == id){
            return tmp;
        }

        tmp = tmp->succ;
    }

    return NULL;
}


bool controllo_prenotazione(char* codice,struct td* attivo){ // controlla che l'orario e data di prenotazione e l'orario e data di attivazione del td corrispondano
    FILE* prenotazioni = fopen("./file/prenotazioni.txt","r+");
    char codice_prenotazione[6];
    char data[9];
    char ora[3];
    time_t rawtime;
    struct tm t_data;
    struct tm* data_oggi;
    struct tm t_ora;
    double sec;
   
  
    time(&rawtime);
    memset(&t_data,0,sizeof(struct tm));
    
    while(!feof(prenotazioni)){
            fscanf(prenotazioni,"%s %s %s %s %*s %*i %*s %*s\n",codice_prenotazione,attivo->tavolo,ora,data); 
            if(strcmp(codice_prenotazione,codice)==0){
                    memset(&t_data,0,sizeof(struct tm));
                    memset(&data_oggi,0,sizeof(struct tm));
                    memset(&t_ora,0,sizeof(struct tm));

                    data_oggi = localtime(&rawtime);
                    strptime(data,"%d-%m-%y",&t_data);
                    strptime(ora,"%H",&t_ora);
                    if(t_data.tm_mday == data_oggi->tm_mday && t_data.tm_mon == data_oggi->tm_mon && t_data.tm_year == data_oggi->tm_year && t_ora.tm_hour == data_oggi->tm_hour){
                        return true;
                    }
                }
    
    }

    strcpy(attivo->tavolo," ");

    return false;
}

bool controllo_tavolo(char* tav,struct td* attivo,int i){ // controlla che il tavolo non sia attivo su due td diversi
    struct td* tmp = attivo;

    while(tmp != NULL){
        if(strncmp(tav,tmp->tavolo,3)==0 && i!=tmp->socket){
            return false;
        }
        tmp= tmp->succ;
    }

    return true;
}

bool check_cmd(struct cmd_ing com){
    if(strcmp(com.cmd,"stat")==0){
            switch(com.par[0]){
            case 'T': case 'a': case 'p': case 's':{


                return true;
                break;
                }
            default :
                {                   return false;
                }
                
        }
    } else if(strcmp(com.cmd,"stop")==0){
        return true;
    }
    printf("Comando non valido...\n");
    return false;
}

struct clienti* rimuovi_clienti(int i,struct clienti* cli){
    struct clienti* tmp = cli;
    struct clienti* elimina = cli->succ;

      

    if(tmp->succ == NULL){
        elimina = cli;
        cli = NULL;
    }


    while(elimina->socket != i){
       
        tmp = elimina;
        elimina = elimina->succ;
    }

   
    tmp->succ = elimina->succ;
    elimina->succ = NULL;

    free(elimina);
    return cli;

}



struct kd* rimuovi_kd(int i,struct kd* cuochi){
    struct kd* tmp = cuochi;
    struct kd* elimina = cuochi->succ;

      

    if(tmp->succ == NULL){
        elimina = cuochi;
        cuochi = NULL;
    }


    while(elimina->socket != i){
       
        tmp = elimina;
        elimina = elimina->succ;
    }

   
    tmp->succ = elimina->succ;
    elimina->succ = NULL;

    free(elimina);
    return cuochi;

}



int pows(int x,int y){
    int ret = 1;
    int count;
    for(count = 1;count<y;count++){
        ret *= x;
    }

    return ret;
}


struct td* rimuovi_td(int i,struct td* tablet){
    struct td* tmp = tablet;
    struct td* elimina = tablet->succ;

      

    if(tmp->succ == NULL){
        elimina = tablet;
        tablet = NULL;
    }


    while(elimina->socket != i){
       
        tmp = elimina;
        elimina = elimina->succ;
    }

   
    tmp->succ = elimina->succ;
    elimina->succ = NULL;

    free(elimina);
    return tablet;

}