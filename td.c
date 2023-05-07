#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <math.h>

typedef enum {false,true} bool;


struct comanda{           // struttura utilizzata per l'immisione e l'invio di comande al server
    char codice[6];
    int quantita;
    struct comanda* succ;
};

struct cmd_ing{         // struttura per l'immisioni dei comandi del tablet
    char cmd[8];
    struct comanda* piatto;
};

struct conto {   // struttura d'appoggio per il calcolo del conto
    char codice[6];
    int quantita;
    int costo;
};

void stampa_help();// stampa le specifiche dei comandi e li descrive
void stampa_menu();// stampa il menu con i vari piatti
bool check_cmd(struct cmd_ing); // controllo validita comandi e comande
bool controlla_piatto(char*);// controlla che il codice del piatto sia presente nel menu
struct comanda* svuota_comanda(struct comanda* testa);// dopo aver inviato la comanda al server, viene utilizzata per cancellare la lista dei piatti
int pows(int,int); // per qualche motivo non trova la pow all'interno della libreria math.h

#define ID_LEN 3
#define LEN_RIGHE 8 


int main(int argc, char* argv[]){
     int ret, sd, len,fdmax,j;
     uint16_t n_piatti=0;

     in_port_t porta = htons(atoi(argv[1]));
    
    struct sockaddr_in srv_addr,my_addr;

    char ok[2];
    char codice[5];
    char id[3];
    char cl[2];
     fd_set master,read_fds;

    char MessaggioIniziale[1024] = "**************BENVENUTO*****************\n\n 1) help\t\t -->mostra i dettagli dei comandi\n 2) menu\t\t --> mostra il menu dei piatti\n 3) comanda\t\t --> invia una comanda\n 4) conto \t\t --> chiede il conto\n\n";

    struct cmd_ing com;
    /* pattern di variabili utili per l'inserimento della comanda*/
    char breaker; 
    char piatto[4];
    int piatti_inviati = 0;
    com.piatto = NULL;
    struct comanda* ultimo=NULL;

    // viariabili utili per la richiesta del conto
    uint16_t piatti_tot;
    int righe_conto;
    struct conto* righe;
    int conto_tot=0;
    char buffer[1024];

    sd = socket(AF_INET,SOCK_STREAM,0);

    memset(&my_addr,0,sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = porta;
    inet_pton(AF_INET,"127.0.0.1",&my_addr.sin_addr);

    ret = bind(sd,(struct sockaddr*)&my_addr,sizeof(my_addr));

    if(ret<0){
        perror("Errore nella bind:");
        exit(1);
    }

    memset(&srv_addr,0,sizeof(srv_addr));
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port = htons(4242);
    inet_pton(AF_INET,"127.0.0.1",&srv_addr.sin_addr);

    ret = connect(sd, (struct sockaddr*)&srv_addr,sizeof(srv_addr));

    if(ret<0){
        perror("Errore in fase di connessione:");
        exit(-1);
    }



    strcpy(id,"td");

    ret = send(sd,(void*)id,ID_LEN,0);

    FD_ZERO(&master);
    FD_ZERO(&read_fds);

    FD_SET(sd,&master);
    FD_SET(0,&master);

    fdmax = sd;

  
    

    if(ret<0){
            perror("Errore in fase di invio id: \n");
            exit(1);
        }

    do{ //controllo codice di accesso
                system("clear");

                printf("Inserisci il codice della prenotazione:\t");
                scanf("%s",codice);

                len = strlen(codice);

                ret = send(sd,(void*)codice,len,0);

                if(ret<0){
                    perror("Errore in fase di invio: \n");
                    exit(-1);
                }

              

                ret = recv(sd,(void*)ok,2,0);


                if(ret<0){
                    perror("Errore in fase di ricezione: \n");
                    exit(-1);
                }

                if(strncmp(ok,"ko",2)==0){
                    printf("Errore, controlla se il codice, l'ora e la data della tua prenotazione sono corretti... premi c e Invio per continuare\n");
                    while(getchar()!='c');// utilizzo per mettere in pausa lo stdout
                }

                
        }while(strncmp(ok,"ok",2)!=0);

    system("clear");
    printf("%s",MessaggioIniziale);

    while(1){

            printf("Premi Enter per digitare un comando...\n");
            read_fds = master;

            ret = select(fdmax+1,&read_fds,NULL,NULL,NULL);
            if(ret<0){
                perror("Errore Select:");
                exit(1);
            }

            for(j = 0;j<= fdmax;j++){
            
                if(FD_ISSET(j,&read_fds)){
                                        if(j==0){
                                             uint16_t len_com;
                                                    do{
                                                        printf("Digita comando: ");
                                                        scanf("%s",com.cmd);
                                                       
                                                        breaker='a'; // inizializzo ad un valore casuale;

                                                        if(strcmp(com.cmd,"comanda")==0){ 
                                                                                                 
                                                                        com.piatto = svuota_comanda(com.piatto);  // resetto e inizializzo la lista dei piatti per poter inserire piu' comande
                                                                        breaker='a';
                                                                        com.piatto = NULL;
                                                                        n_piatti = 0; 
                                                                        
                                                                        while(breaker!='\n'){ // prendo l'ordine di piatti come lista, contando quanti sono per poi fare un vettore nel server
                                                                                        int esp;
                                                                                        struct comanda* new_order = (struct comanda*) malloc(sizeof( struct comanda));
                                                                                        scanf("%s",new_order->codice);
                                                                                       
                                                                                        scanf("%c",&breaker);

                                                                                        new_order->quantita = 0;
                                                                                        len = strlen(new_order->codice);
                                                                                        
                                                                                        for(esp = 3;esp<len;esp++){  // trasformo la quantita presa come char in un valore intero
                                                                                            new_order->quantita += (new_order->codice[esp]-48) * pows(10,len - esp); 
                                                                                            new_order->codice[esp]= '\0';
                                                                                        }
                                                                                      
                                                                                        if(com.piatto == NULL){
                                                                                            new_order->succ = NULL;
                                                                                            com.piatto = new_order;
                                                                                            ultimo = com.piatto;
                                                                                        } else {
                                                                                            ultimo->succ = new_order;
                                                                                            ultimo = ultimo->succ;
                                                                                            ultimo->succ = NULL;
                                                                                        }
                                                                                        n_piatti++;              

                                                                                } 
                                                        }

                                                    }while(check_cmd(com) == false);



                                                    if(strcmp(com.cmd,"help")==0){
                                                                stampa_help();
                                                    }else if(strcmp(com.cmd,"menu")==0){
                                                                stampa_menu();
                                                    }else if(strcmp(com.cmd,"comanda")==0){
                                                                struct comanda* dainviare = com.piatto; 
                                                                 
                                                                len = strlen(com.cmd);

                                                                len_com = htons(len);

                                                                ret = send(sd,(void*)&len_com,sizeof(uint16_t),0);  // invio la lunghezza del comando e poi il comando

                                                                ret = send(sd,(void*)com.cmd,len,0);

                                                                if(ret<0){
                                                                    perror("Errore in fase di invio comando: \n");
                                                                    exit(1);
                                                                }

                                                                n_piatti = htons(n_piatti); // invio il numero di piatti ordinati

                                                                ret = send(sd,(void*)&n_piatti,sizeof(uint16_t),0);

                                                                n_piatti = ntohs(n_piatti);


                                                                for(piatti_inviati =0;piatti_inviati<n_piatti;piatti_inviati++){ // invio i piatti ordinati uno alla volta facendo la serializzazione di ogni piatto

                                                                                            sprintf(piatto,"%s%i",dainviare->codice,dainviare->quantita);

                                                                                            len = strlen(piatto);


                                                                                            len_com = htons(len);

                       

                                                                                            ret = send(sd,(void*)&len_com,sizeof(uint16_t),0);

                                                                                            ret = send(sd,(void*)piatto,len,0);

                                                                                            if(ret<0){
                                                                                                perror("Errore in fase di invio piatto: \n");
                                                                                                exit(1);
                                                                                            }

                                                                                            dainviare = dainviare->succ;


                                                                }


                                                              

                                                             


                                                }else if(strcmp(com.cmd,"conto")==0){

                                                                    conto_tot = 0;
                                                                    
                                                                    len = strlen(com.cmd);

                                                                     
                                                                    len_com = htons(len);

                                                                    ret = send(sd,(void*)&len_com,sizeof(uint16_t),0);

                                                                    ret = send(sd,(void*)com.cmd,len,0);

                                                                    if(ret<0){
                                                                            perror("Errore in fase di invio comando: \n");
                                                                            exit(1);
                                                                        }

                                                                    ret = recv(sd,(void*)&piatti_tot,sizeof(uint16_t),0); // ricezioni dei piatti totali ordinati dal tablet


                                                                    if(ret<0){
                                                                            perror("Errore in fase di ricezione numero piatti: \n");
                                                                            exit(1);
                                                                        }

                                                                    piatti_tot = ntohs(piatti_tot);

                                                                    if(piatti_tot == 0){
                                                                        printf("Non hai effettuato nessun ordine...\n");
                                                                        break;
                                                                    }

                                                                    righe = (struct conto*) malloc(sizeof(struct conto)*piatti_tot);

                                                                    for(righe_conto = 0;righe_conto<piatti_tot;righe_conto++){ // parsing dello scontrino inviato dal server
                                                                                    ret = recv(sd,(void*)buffer,LEN_RIGHE,0);

                                                                                    if(ret<0){
                                                                                            perror("Errore in fase di ricezione: \n");
                                                                                            exit(1);
                                                                                    }

                                                                                   
                                                                                    sscanf(buffer,"%c%c %i %i",&righe[righe_conto].codice[0],&righe[righe_conto].codice[1],&righe[righe_conto].quantita,&righe[righe_conto].costo);
                                                                                    
                                                                                    printf("%c%c %i %i\n",righe[righe_conto].codice[0],righe[righe_conto].codice[1],righe[righe_conto].quantita,righe[righe_conto].costo);

                                                                                    conto_tot += righe[righe_conto].costo;

                                                                    }

                                                                    printf("Totale: %i\n",conto_tot);

                                                }
                                        }else{ // gestione per la chiusura del server
                                                len = strlen(cl);

                                                ret = recv(sd,(void*)&cl,len,0);

                                                if(strncmp(cl,"cl",2)==0 || ret == 0 ){
                                                    printf("Disconnessione\n");
                                                    close(sd);
                                                    exit(1);
                                                }

                                        }

                    }

           
            }
    }


   

    return 0;

}



/****************FUNZIONI*****************/

void stampa_help(){

    printf("\n*******************COMANDI***************\n\n");
    printf("menu:\t non ha bisogno di nessun parametro; eseguendolo viene visualizzato il menu del giorno, ogni riga presenta un piatto ed e` formata dal codice del piatto (es. A1)\n dal nome del piatto (es. Antipasto di Terra) e dal costo del piatto (es. 7) \n\n");
    printf("comanda:\t prende in ingresso il codice del piatto desiderato (es. A1)\n piu` le quantita` (es. 3) separate da \" - \" (es. A1-3)\n\n");
    printf("conto:\t richiede il conto del tavolo al server\n\n");

    return;
}

void stampa_menu(){

    printf("\n********************MENU********************");

    printf("\n\n");
    
    printf("A1 - Antipasto di terra\t\t\t\t7\nA2 - Antipasto di mare\t\t\t\t8\nP1 - Pasta alle vongole\t\t\t\t10\nP2 - Rigatoni all' amatriciana\t\t\t6\nS1 - Frittata di calamari\t\t\t20\nS2 - Arrosto misto\t\t\t\t15\nD1 - Baba`\t\t\t\t\t8\nD2 - Sorbetto all'amarena\t\t\t11\nL1 - Amaro Lucano\t\t\t\t7\nL2 - Grappa barricata\t\t\t\t12\n\n");

    return;

}


struct comanda* svuota_comanda (struct comanda* testa){
    struct comanda * tmp = testa;
    struct comanda * elimina = testa;

    while(elimina != NULL){
       
        tmp = elimina->succ;
       
        free(elimina);
         elimina = tmp;
    }
    testa = NULL; 
    return testa;
    

}


bool check_cmd(struct cmd_ing com){
    
        if(strcmp(com.cmd,"menu")==0 || strcmp(com.cmd,"conto")==0|| strcmp(com.cmd,"help")==0){ // controllo se il comando inserito e` corretto
                return true;
        } else if(strcmp(com.cmd,"comanda")==0){
            if(com.piatto == NULL){
                printf("Nessun piatto inserito\n");
                strcpy(com.cmd," ");
                return false;
            } else{
                struct comanda* tmp = com.piatto;

               
                while(tmp!=NULL){  // controllo piatto per piatto se il formato immesso e` corretto
                    if(controlla_piatto(tmp->codice)==false){
                        printf("Piatto %c%c non presente nel menu\n",tmp->codice[0],tmp->codice[1]);
                        strcpy(com.cmd," ");
                        return false;
                    } else if(tmp->codice[2]!='-'){
                        printf("Errore nel formato dell'ordine %s%i, Formato corretto: %c%c-%i\n",tmp->codice,tmp->quantita,tmp->codice[0],tmp->codice[1],tmp->quantita);
                        strcpy(com.cmd," ");
                        return false;
                    } else if(tmp->quantita<=0){
                        printf("Errore! Le quantita del piatto richiesto devo essere maggiori o uguali 1, controllare il piatto %s%i\n",tmp->codice,tmp->quantita);
                        strcpy(com.cmd," ");
                        return false;
                    }
                    tmp = tmp->succ;
                }
                  return true;
            }
        }

        printf("Comando non valido: %s\n",com.cmd);
        while (getchar() != '\n'); // vuoto lo stdin
        strcpy(com.cmd," ");
        return false;
}

bool controlla_piatto(char* piatto){
    switch(piatto[0]){
        case 'A':
        case 'P':
        case 'S':
        case 'D':
        case 'L':
            switch(piatto[1]){
                case '1':
                case '2':
                    return true;
                break;
            }
            break;
    }
    return false;
}

int pows(int x,int y){
    int ret = 1;
    int count;
    for(count = 1;count<y;count++){
        ret *= x;
    }

    return ret;
}