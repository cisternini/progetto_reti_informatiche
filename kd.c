#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

typedef enum{false,true} bool;
 
struct cmd_ing{  // struttura per la gestione dei comandi
    char cmd[5];
    char comanda[6];
    char tavolo[4];
};

struct righe{  // struttura per la gestione dei piatti ordinati
    char codice[3];
    int quantita;
    struct righe* succ;
};

struct comanda{ // struttura per la gestione delle comande in preparazione dal kitchen device
    char c_comanda[5]; // codice comanda
    char c_tavolo[4]; // codice tavolo
    char stato; // stato della comanda
    int n_piatti; // numero di piatti
    struct righe* piatti; // elenco di piatti in preparazione
    struct comanda* succ;
};


bool check_cmd(struct cmd_ing); // controllo formato dei comandi
struct righe* aggiungi_piatto(struct righe* piatto, struct righe* testa); // aggiungo il piatto alla comanda in preparazione
struct comanda* aggiungi_comanda(struct comanda* com, struct comanda* testa); // aggiunge una nuova comanda a quelle in preparazione
struct comanda* elimina_comanda(char* comanda,char* tavolo, struct comanda* testa); // elimina le comdande dalla lista
bool in_lista_preparazione(char* codice,char* tavolo,struct comanda* testa);// controlla se la lista richiesta (dal comando show) e' in preparazione

#define ID_LEN 3 // lunghezza dell'id
#define CL_LEN 2 // lunghezza per i comandi di chiusura
#define COM_LEN 10 // lunghezza per le comande in preparazione e servizio


int main(int argc, char* argv[]){
     int ret, sd, len,segnali,fdmax,j;
    uint16_t n_ordini; // numero di ordini in attesa 
    in_port_t porta = htons(atoi(argv[1])); 

    fd_set master,read_fds;
    struct sockaddr_in srv_addr,my_addr;

    struct comanda* in_preparazione = NULL; // lista delle comande in preparazione

    char id[ID_LEN];
    struct cmd_ing com; // comandi in ingresso
    char cl[CL_LEN]; // conferma di chiusura

    char MessaggioIniziale[1024] = "\n1) take\t\t-->accetta una comanda\n2) show\t\t--> mostra le comande accettate (in preparazione)\n3) ready\t\t--> imposta lo stato della comanda\n\n";

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
        perror("Errore in fase di connessione: ");
        exit(-1);
    }

    FD_ZERO(&master);
    FD_ZERO(&read_fds);

    FD_SET(sd,&master);
    FD_SET(0,&master);

    fdmax = sd;

    printf("\nComande insospese: \n");

    printf("%s\n",MessaggioIniziale);  

    strcpy(id,"kd");


    ret = send(sd,(void*)id,ID_LEN,0);


    while(1){
        printf("Premere Invio per digitare un comando....\n");
        read_fds = master;
         
        ret = select(fdmax+1,&read_fds,NULL,NULL,NULL);
        if(ret<0){
            perror("Errore Select:");
            exit(1);
        }

        
            

        for(j = 0;j<= fdmax;j++){

                   
                    if(FD_ISSET(j,&read_fds)){
                                if(j==sd){

                                            ret = recv(sd,(void*)&cl,CL_LEN,0);

                                            if(strncmp(cl,"cl",2)==0 || ret == 0){ // effettua la disconnessione dopo una il comando stop del server oppure dopo una chiusura improvvisa
                                                printf("Disconnessione\n");
                                                close(sd);
                                                exit(1);
                                            } else {
                                                sscanf(cl,"%i",&n_ordini); // fa il parsing del numero di ordini
                                                //system("clear");

                                                printf("\nComande insospese: ");
                                                for(segnali = 0;segnali<n_ordini;segnali++){ // aggiorna il numero di ordini insospesi
                                                    printf("%c",'*');
                                                }
                                                printf("\n\n");
                                            }

                                            printf("%s\n",MessaggioIniziale);  

                                        
                                        } else if(j==0){
                                                            char prenotato[2];
                                                            strcpy(prenotato,"ok");

                                                            do{
                                                                printf("Digita un comando: ");

                                                                scanf("%s",com.cmd);

                                                                if(strcmp(com.cmd,"take")==0){
                                                                    FILE* comande = fopen("./file/comande.txt","r+");
                                                                    struct comanda* nuova_comanda = (struct comanda*)malloc(sizeof(struct comanda));
                                                                    char comanda_accettata[COM_LEN]; // comanda accettata da inviare, contiene il nome della comanda e il codice del tavolo
                                                                    fpos_t cur; // posizione corrente nel file
                                                                    int num_piatto; 
                                                                    char tipo;
                                                                    struct righe* prova;
                                                                    if(n_ordini > 0){ // controllo numero ordini
                                                                        while(!feof(comande)){ // ciclo per la ricerca di comande da eseguire
                                                                            int k;
                                                                            fscanf(comande,"%c",&tipo);
                                                                            if(tipo == 'c'){ 
                                                                                fgetpos(comande,&cur); // salvo la posizione al carattere di controllo
                                                                                fscanf(comande,"%s %s %c %i\n",nuova_comanda->c_comanda,nuova_comanda->c_tavolo,&nuova_comanda->stato,&nuova_comanda->n_piatti);

                                                                                if(nuova_comanda->stato == 'a'){ // controllo dello stato della comanda, salvo la prima comanda in attesa, modifico lo stato della comanda 
                                                                                    fsetpos(comande,&cur); // resetto il puntatore del file al punto salvato
                                                                                    nuova_comanda->stato = 'p'; 
                                                                                    fprintf(comande," %s %s %c %i\n",nuova_comanda->c_comanda,nuova_comanda->c_tavolo,nuova_comanda->stato,nuova_comanda->n_piatti); // stampo nel file
                                                                                    break;
                                                                                }
                                                                                else {
                                                                                    for(k=0;k<nuova_comanda->n_piatti;k++){
                                                                                        fscanf(comande,"%*c %*c%*c %*i\n"); // salta le righe della comanda che non mi interessano
                                                                                    }
                                                                                }
                                                                            } 
                                                                        }
                                                                    
                                                                        printf("%s %s\n",nuova_comanda->c_comanda,nuova_comanda->c_tavolo);

                                                                        nuova_comanda->piatti = NULL;
                                                                        nuova_comanda->succ = NULL;
                                                                        for(num_piatto = 0;num_piatto<nuova_comanda->n_piatti;num_piatto++){ // salva i piatti nella lista della comande in preparazione del kitchen device
                                                                                            struct righe* piatto = (struct righe*)malloc(sizeof(struct righe));
                                                                                            fscanf(comande,"%*c %c%c %i\n",&piatto->codice[0],&piatto->codice[1],&piatto->quantita);
                                                                                            piatto->succ = NULL;  
                                                                                            printf("%c%c %i\n",piatto->codice[0],piatto->codice[1],piatto->quantita);                               
                                                                                            nuova_comanda->piatti = aggiungi_piatto(piatto,nuova_comanda->piatti);
                                                                        }

                                                                        printf("\n");
                                                                        in_preparazione = aggiungi_comanda(nuova_comanda,in_preparazione); // aggiungo la comanda alla lista in preparazione

                                                                        sprintf(comanda_accettata,"%s %s %c",nuova_comanda->c_comanda,nuova_comanda->c_tavolo,'p');

                                                                        printf("%s",comanda_accettata);

                                                                        ret = send(sd,(void*)comanda_accettata,COM_LEN,0);

                                                                        if(ret<0){
                                                                            perror("Errore nella send:");
                                                                            continue;
                                                                        }

                                                                        fclose(comande);

                                                                        
                                                                        ret = send(sd,(void*)prenotato,CL_LEN,0); // invia il segnale al server per decrementare il numero di ordini
                                                                    } else {
                                                                        printf("Non ci sono comande pendenti...\n");
                                                                        continue;
                                                                    }


                                                                    
                                                                } else if(strcmp(com.cmd,"show")==0){
                                                                    struct comanda* tmp_comande = in_preparazione;
                                                                    struct righe* tmp_righe;

                                                                    if(in_preparazione == NULL){
                                                                        printf("Non ci sono comande in preparazione...\n");
                                                                        continue;
                                                                    }
                                                                    printf("\n");
                                                                    while(tmp_comande!=NULL){ // mostra le comande in preparazione nel kitchen device
                                                                        tmp_righe = tmp_comande->piatti;
                                                                        printf("%s %s\n",tmp_comande->c_comanda,tmp_comande->c_tavolo);
                                                                        
                                                                        while(tmp_righe!=NULL){
                                                                                printf("%s %i\n",tmp_righe->codice,tmp_righe->quantita);
                                                                                tmp_righe = tmp_righe->succ;
                                                                        }

                                                                        tmp_comande = tmp_comande->succ;
                                                                    }
                                                                    printf("\n");

                                                                }else if(strncmp(com.cmd,"ready",5)==0){
                                                                    
                                                                    FILE* comande = fopen("./file/comande.txt","r+");
                                                                    char parametro[9]; // preleva la comanda e il tavolo da settare
                                                                    char codice[4]; // variabile d'appoggio per il confronto
                                                                    char tavolo[3]; // come sopra
                                                                    char comanda_servita[COM_LEN];
                                                                    char tipo; // ----
                                                                    char stato; // ----
                                                                    int piatti; //----
                                                                    fpos_t cur; // salva la posizione corrente nel file
                                                                    int j; // indice per il parsing
                                                                    int k;


                                                                    scanf("%s",parametro);
                                                                   
                                                                    for(j=0;parametro[j]!='-';j++){ // parse della comanda
                                                                        com.comanda[j] = parametro[j];
                                                                    }
                                                                    com.comanda[j] = '-';
                                                                    j++;
                                                                    com.comanda[j] = '\0';
                                                                   
                                                                    for(k=0;k<3;k++){ // parse del tavolo
                                                                        com.tavolo[k] = parametro[k+j];
                                                                    }
                                                                  
                                                                    if(check_cmd(com)==false) // controllo se il parametro e' corretto
                                                                    {
                                                                        printf("Errore nell'inserimento della comanda o del tavolo...\n");
                                                                        fclose(comande);
                                                                        break;
                                                                    }

                                                                    while(1){
                                                                        int k;
                                                                       
                                                                        if(feof(comande)){
                                                                            printf("Comanda %4s del tavolo %s non trovata\n",com.comanda,com.tavolo);
                                                                            break;
                                                                        }
                                                                        fscanf(comande,"%c",&tipo);
                                                                        if(tipo == 'c'){
                                                                            fgetpos(comande,&cur);// salvo la posizione della comanda 
                                                                            fscanf(comande,"%s %s %c %i\n",codice,tavolo,&stato,&piatti); // prelevo la comanda
                                                                            
                                                                            if(strncmp(com.comanda,codice,4)==0 && strncmp(com.tavolo,tavolo,3)==0){ // controllo lo stato
                                                                                    if(stato == 'a'){
                                                                                        printf("Questa comanda e' ancora in attesa...\n");
                                                                                        break;
                                                                                    }
                                                                                    if(stato == 's'){
                                                                                        printf("Questa comanda e' gia' in servizio...\n");
                                                                                        break;
                                                                                    }
                                                                                    if(in_lista_preparazione(codice,tavolo,in_preparazione) == false){ // o se e' in preparazione nel kitchen device corrente
                                                                                        printf("La comanda non e' in preparazione presso questo kitchen device...\n");
                                                                                        break;
                                                                                    }
                                                                                    fsetpos(comande,&cur); // setto il cursore
                                                                                    stato = 's'; // modifico lo stato
                                                                                    com.comanda[j-1] = '\0';
                                                                                    fprintf(comande," %s %s %c %i",com.comanda,com.tavolo,stato,piatti);  // salvo sul file                                                                     
                                                                                    in_preparazione = elimina_comanda(codice,tavolo,in_preparazione);
                                                                                    printf("\nCOMANDA IN SERVIZIO\n");

                                                                                    sprintf(comanda_servita,"%s %s %c",com.comanda,com.tavolo,'s');

                                                                                    ret = send(sd,(void*)comanda_servita,COM_LEN,0);

                                                                                                    
                                                                                    if(ret<0){
                                                                                        perror("Errore nella send:");
                                                                                        continue;
                                                                                    }                                                                        
                                                                                                                                                    
                                                                                    break;
                                                                                }
                                                                            else {
                                                                                for(k=0;k<piatti;k++){
                                                                                    fscanf(comande,"%*c %*c%*c %*i\n"); // salto le righe se non sono righe di comande
                                                                                }
                                                                            }
                                                                        } 
                                                                    }

                                                                    com.comanda[j-1] = '-';
                                                                    fclose(comande);
                                                                }

                                                            }while(check_cmd(com)==false);
                                            }

    
                                }
                        }

        }
        return 0;
    
}



/****************************FUNZIONI************************/
struct righe* aggiungi_piatto(struct righe* piatto, struct righe* testa){
    struct righe* tmp = testa;

    if(tmp==NULL){
        testa = piatto;
     }else{
        while(tmp->succ!=NULL){
            tmp = tmp->succ;
        }
        piatto->succ = NULL;
        tmp->succ = piatto;
       
    }

     return testa;
}


struct comanda* aggiungi_comanda(struct comanda* com, struct comanda* testa){
    struct comanda* tmp = testa;

    if(tmp==NULL){
        testa = com;
    }else{
        while(tmp->succ!=NULL){
            tmp = tmp->succ;
        }
        com->succ = NULL;
        tmp->succ = com;
    }

    return testa;
}


struct comanda* elimina_comanda(char* comanda,char* tavolo, struct comanda* testa){
        struct comanda* tmp = testa;
        struct comanda* elimina = testa;
        struct righe* tmp_piatti;
        struct righe* elimina_piatti;

        if(testa == NULL){
            return testa;
        } else {

            while(strncmp(comanda,elimina->c_comanda,4)!=0 || strncmp(tavolo,elimina->c_tavolo,3)!=0){
                tmp = elimina;
                elimina = elimina->succ;
            }

            tmp_piatti = elimina->piatti;
            elimina_piatti = elimina ->piatti;

            while(elimina_piatti!=NULL){
                tmp_piatti = elimina_piatti->succ;
                free(elimina_piatti);
                elimina_piatti = tmp_piatti;
            }
            elimina->piatti = NULL;
            
            if(elimina == testa){
                testa = elimina->succ;
            } else {
                tmp->succ = elimina->succ;
            }

           
            elimina->succ = NULL;
            free(elimina);
            
        }
    
    return testa;

}

bool check_cmd(struct cmd_ing com){
    if(strcmp(com.cmd,"take")==0 || strcmp(com.cmd,"show")==0){
        return true;
    } else if(strncmp(com.cmd,"ready",5)==0){

        if(strncmp(com.comanda,"com",3)!=0 || (com.comanda[3]<'1'|| com.comanda[3]>'9')|| com.comanda[4]!='-'){
            return false;           
        }

        switch(com.tavolo[0]){
            case 'T':
                switch (com.tavolo[1])
                {
                case  '1': case '2': case '3':
                    switch(com.tavolo[2]){
                       case  '1': case '2': case '3':
                       return true;
                       break;
                    }
                    break;
                }
            break;
            default:
                return false;
            break;
        }

        return true;
    }

    printf("Comando non valido...\n");
    while(getchar()!='\n');
    return false;
}


bool in_lista_preparazione(char* codice,char* tavolo,struct comanda* testa){
       struct comanda* tmp = testa;

       while(tmp!=NULL){
            if(strncmp(codice,tmp->c_comanda,4)==0 && strncmp(tavolo,tmp->c_tavolo,3)==0){
                return true;
            }
            tmp = tmp->succ;
       }

       return false;

}