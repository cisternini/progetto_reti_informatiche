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


typedef enum {false,true} bool;


struct cmd_ing { 
    char ora[3];// struttura per la gestione dei comandi con i parametri necessari
    char cmd[5];
    char data[9];
    char nome[30];
    int num_per;
    int opz; 
};

struct tavoli{ // struttura per la gestione dei tavoli contenente : codice, posizione nella sala, una descrizione, l'opzione a cui corrisponde il tavolo
    char t_nome[4];
    char posizione[6];
    char descrizione[20];
    int opz;
   
};

bool check_cmd(struct cmd_ing com,int t); // funzione di controllo per i comandi e il formato dei parametri
void nuovo_tavolo(struct tavoli* tav, struct tavoli new_tav); // funzione per l' aggiunta dei tavoli
void cancella_tavoli(struct tavoli* tav);// funzione per la cancellazione della lista dei tavoli trovati
bool controllo_data(char* data); // funzione per controllare la validita della data richiesta
bool controllo_ora(char * ora); // funzione per controllare la validita dell'orario richiesto

#define ID_LEN 3 // lunghezza dell'identificativo del client
#define BUFFER_LEN 1024 // utilizzo buffer molto lunghi non sapendo quanto possa essere lunga stringa dei parametri immessi
#define DIFFERENZA_GIORNO 83400.0 // costante il controllo dell'orario
#define ORA_APERTURA 16
#define ORA_CHIUSURA 23

int main(int argc, char* argv[]){ 
    int ret, sd, len;
    uint16_t t=0; // numero di tavoli trovati
    in_port_t porta = htons(atoi(argv[1]));
   
    struct sockaddr_in srv_addr,my_addr;
    char MessaggioIniziale[1024] = "******************************BENVENUTO********************************\n\n - find\t\t Inserisci la prenotazione che vuoi effettuare [es. find cognome numero di persone data (GG-MM-AA) orario (HH)]\n - book\t\t Scegli fra i tavoli proposti [es book numero opzione]  \n - esc\t\t Chiudi la sessione\n";
    struct cmd_ing com;// comando da prendere in input
    struct tavoli* tav = NULL; // tavoli trovati prenotabili
    char buffer[BUFFER_LEN];// sgringa dei dati da inviare
    
    char id[ID_LEN]; // id del client (conterra' "cli")
   
    
    sd = socket(AF_INET,SOCK_STREAM,0);

    memset(&my_addr,0,sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = porta;
    inet_pton(AF_INET,"127.0.0.1",&my_addr.sin_addr);

    ret = bind(sd,(struct sockaddr*)&my_addr,sizeof(my_addr));


    memset(&srv_addr,0,sizeof(srv_addr));
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port = htons(4242);
    inet_pton(AF_INET,"127.0.0.1",&srv_addr.sin_addr);

    ret = connect(sd, (struct sockaddr * )&srv_addr,sizeof(srv_addr));


    if(ret<0){
        perror("Errore in fase di connessione");
        exit(-1);
    }

    strcpy(id,"cli");

             
    ret = send(sd,(void*)id,ID_LEN,0); //segnalo al server che chi sta richiedendo la connessione e' un cliente
    if(ret<0){
                perror("Errore in fase di invio id: \n");
                exit(1);
            }

    

    printf("%s\n",MessaggioIniziale);   //Stampa le scelte inziali e permette l'inserimento di un comando
   
   
    
    while(1){
                            // inserimento dei comandi con relativi controlli
                            do{
                                printf("Inserisci un comando:\t");
                                scanf("%s",com.cmd);
                                
                                if(strcmp(com.cmd,"book")==0){
                                      scanf("%i",&com.opz);
                                } else if(strcmp(com.cmd,"find")==0){
                                
                                    scanf("%s",com.nome);
                                    scanf("%i",&com.num_per);
                                    scanf("%s",com.data);
                                    scanf("%s",com.ora);

                                }
                         
                            }while(check_cmd(com,t)==false);

                            if(strcmp(com.cmd,"find")==0){
                                            int i;

                                            sprintf(buffer,"%s %s %i %s %s",com.cmd,com.nome,com.num_per,com.data,com.ora); // serializzazione prenotazione

                                           

                                            ret =  send(sd,(void*)buffer,BUFFER_LEN,0);

                                            if(ret<0){
                                                perror("Errore in fase di invio comando find: \n");
                                                    exit(1);
                                            }

                                            ret = recv(sd,(void*)&t,sizeof(uint16_t),0);

                                            t = ntohs(t);

                                            

                                            if(ret<0){
                                                    perror("Errore in fase di ricezione: \n");
                                                    exit(-1);
                                            }

                                            if(t!=0){ 
                                                if(tav != NULL){  // se ci sono tavoli salvati li cancello
                                                    free(tav);
                                                    tav = NULL;
                                                }
                                                tav = (struct tavoli*) malloc(sizeof(struct tavoli)*t);

                                                for(i=0;i<t;i++){ //aggiungo alla lista dei tavoli che e' possibile prenotare;
                                                            ret = recv(sd,(void*)buffer,BUFFER_LEN,0);
                                                        sscanf(buffer,"%i %s %s %s",&tav[i].opz,tav[i].t_nome,tav[i].posizione,tav[i].descrizione);
                                                   
                                                }

                                                printf("\nScegli fra i seguenti tavoli:\n\n");
                                                for(i = 0;i<t;i++){//stampa la lista dei tavoli disponibili
                                                    printf("%i) %s %s %s\n",tav[i].opz,tav[i].t_nome,tav[i].posizione,tav[i].descrizione);
                                                }
                                                
                        

                                            }
                                            else{
                                                printf("\nNon ci sono tavoli disponibili per l'orario scelto\n");
                                            }
                                            
                            }
                            else if(strcmp(com.cmd,"book")==0 && tav != NULL){
                                            char codice[6];
                                            char ok[3];
                                            sprintf(buffer,"%s %i",com.cmd,com.opz);
                                        
                                            ret =  send(sd,(void*)buffer,BUFFER_LEN,0); // invio dell' opzione scelta

                                            if(ret<0){
                                                perror("Errore in fase di invio comando book: \n");
                                                    exit(1);
                                            }
                                            
                                            ret = recv(sd,(void*)buffer,BUFFER_LEN,0); // ricezione conferma
                                        
                                            sscanf(buffer,"%s %s",ok,codice);
                                                                                

                                            if(ret<0){
                                                    perror("Errore in fase di ricezione: \n");
                                                    exit(-1);
                                            }

                                            if(strncmp(ok,"ok",2)==0){
                                                printf("\nPRENOTAZIONE EFFETTUATA\n CODICE PRENOTAZIONE: %s\n TAVOLO: %s\n POSIZIONE: %s\n",codice,tav[com.opz-1].t_nome,tav[com.opz-1].posizione);
                                            }
                                            else {
                                                printf("\nERRORE NELLA PRENOTAZIONE");
                                            }

                                            free(tav);
                                            tav = NULL;
                                            t = 0;
                            } else if(strcmp(com.cmd,"esc")==0){
                                close(sd);
                                break;
                            }
            printf("\n");
                        
                
            }
    close(sd);
   
    return 0;
}








//****************************FUNZIONI*************************

bool check_cmd(struct cmd_ing com,int t){
    int i;
    
    if(strcmp(com.cmd,"find")==0){
                                for(i = 0;i<strlen(com.nome);i++){  // controllo che nel nome non ci siano caratteri speciali
                                    if((com.nome[i]<65 || (com.nome[i]>90  && com.nome[i]<97) || com.nome[i]>122)){
                                        printf("Carattere non permesso: %i\n",com.nome[i]);
                                        return false;
                                    }
                                }

                                if(com.num_per<1 || com.num_per>20){  // il limite per il numero di tavoli e` 20  
                                    printf("Numero di persone superiore al limite dei tavoli");
                                    return false;
                                }

                                if((com.ora[0]<'0' || com.ora[0]>'2')){ // controllo del formato ora
                                    if(com.ora[0] == '0' || com.ora[0]=='1')
                                                if(com.ora[1]<'0' || com.ora[1]>'9')
                                                                            return false;
                                    else 
                                                if(com.ora[1]<'0' || com.ora[1]>'4')
                                                                            return false; 
                                    printf("Formato ora errato\n");
                                    }

                                if(com.data[3]<'0' || com.data[3]>'1'){ // da qui in poi si tratta di controlli per il formato della data,
                                    printf("Formato mese errato\n"); //  quindi si considerno tutti i mesi con il loro numero di giorni esatto
                                    return false;
                                }

                                if(com.data[3]=='0'){
                                        if((com.data[4]<'0' || com.data[4]>'9'))
                                            {
                                                printf("Formato mese errato\n");
                                                return false;  
                                            }        
                                } else if(com.data[3]=='1'){
                                        if((com.data[4]<'0' || com.data[4]>'2'))
                                        {
                                                printf("Formato mese errato\n");
                                                return false; 
                                        }  
                                }
                                switch(com.data[3]){
                                        case '0':
                                            switch(com.data[4]){
                                                    case '1': case '3':case '5': case '7':case '8':
                                                            
                                                                if(com.data[0] >='0' && com.data[0]<='2'){
                                                                        if(com.data[1]<'0' || com.data[1]>'9')  {printf("primo\n");return false;}
                                                                }else if(com.data[0]=='3'){
                                                                        if(com.data[1]<'0' || com.data[1]>'1')  {return false;}
                                                                } else {
                                                                    printf("Controlla che il formato del mese sia corretto...\n");
                                                                    return false;
                                                                }
                                                    break;
                                                    case '4': case '6': case '9':
                                                                if(com.data[0]>='0' && com.data[0]<='2'){
                                                                        if(com.data[1]<'0' || com.data[1]>'9')  {return false;}
                                                                }else if(com.data[0]=='3'){
                                                                        if(com.data[1]!='0')  {return false;}
                                                                }else {
                                                                    printf("Controlla che il formato del mese sia corretto...\n");
                                                                    return false;
                                                                }
                                                    break;
                                                    case '2':
                                                                if(com.data[0]>='0' || com.data[0]<='1'){
                                                                        if(com.data[1]<'0' || com.data[1]>'9')  {return false;}
                                                                }else if(com.data[0]=='2'){
                                                                        if(com.data[1]<'0' || com.data[1]>'8')  {return false;}
                                                                }else{
                                                                    printf("Controlla che il formato del mese sia corretto...\n");
                                                                    return false;
                                                                }
                                            }
                                            break;
                                        case '1':
                                            
                                            switch(com.data[4]){
                                                    case '0': case '2':
                                                                if(com.data[0]>='0' && com.data[0]<='2'){
                                                                        if(com.data[1]<'0' || com.data[1]>'9')  {printf("primo\n");return false;}
                                                                }else if(com.data[0]=='3'){
                                                                        if(com.data[1]<'0' || com.data[1]>'1')  {return false;}
                                                                } else {
                                                                    printf("Controlla che il formato del mese sia corretto...\n");
                                                                    return false;
                                                                }
                                                    break;
                                                    case '1':
                                                                if(com.data[0]>='0' && com.data[0]<='2'){
                                                                        if(com.data[1]<'0' || com.data[1]>'9')  {return false;}
                                                                }else if(com.data[0]=='3'){
                                                                        if(com.data[1]!='0')  {return false;}
                                                                }else {
                                                                    printf("Controlla che il formato del mese sia corretto...\n");
                                                                    return false;
                                                                }
                                                    break;
                                            }
                                        break;         
                                }
                                

                                if(com.data[2]!='-' || com.data[5]!='-' ){
                                    printf("Formato data non valido. Formato corretto GG-MM-AA\n");
                                    return false;
                                }    

                                if((com.data[6]<'2'||com.data[6]>'6')||((com.data[7]<'3'||com.data[7]>'8'))){ // controllo che l'anno sia compreso tra il 23 - 68 poiche' il formato utilizzato per 
                                    return false;                                                             // il formato della data  nella funzione strptime prevede che da 00 - 68 sia considerato 20xx mentre da 69 - 99 e' considerato come 19xx                               
                                } 

                                if(controllo_ora(com.ora)==false){
                                    printf("L'ora selezionata non e' disponibile perche' siamo chiusi...\n");
                                    return false;
                                }
                            
                                if(controllo_data(com.data) == false){
                                    printf("La data selezionata e` gia` passata,non e` possibile prenotare in questa da...\n");
                                    return false;
                                }
                                
                            return true;
    } else if (strcmp(com.cmd,"book")==0){
                            if(com.opz>t || com.opz<0){
                                if(t==0){
                                    printf("Errore non hai rischiesto nessuna prenotazione...\n");
                                } else {
                                    printf("Opzione non presente...\n"); 
                                }
                                return false;  
                            }
                            
                            return true;     
    } else if(strcmp(com.cmd,"esc")==0){
        return true;
    }

    printf("Comando non valido: %s\n",com.cmd);
    while (getchar() != '\n'); // svuoto la stdin in caso di errore dei comandi
    return false;
}


bool controllo_data(char* data){ // controllo che la data inserita non sia precedente alla data odierna
    time_t rawtime;
    struct tm t_data;
  
    time_t d;
    double sec;

    time(&rawtime);
    memset(&t_data,0,sizeof(struct tm));
    strptime(data,"%d-%m-%y",&t_data);
  
    d = mktime(&t_data);
    sec = difftime(d,rawtime);

  
    if(sec<-DIFFERENZA_GIORNO){
        return false;
    } else {
        return true;
    }    
     
}

bool controllo_ora(char * ora){ // controllo che l'ora richiesta sia dopo il momento della prenotazione e all'interno l'orario di apertura 
    
    struct tm t_ora;
    struct tm* ora_oggi;
    time_t rawtime;
    time(&rawtime);
    ora_oggi = localtime(&rawtime);
    memset(&t_ora,0,sizeof(struct tm));
    strptime(ora,"%H",&t_ora);

    if(t_ora.tm_hour<ORA_APERTURA || t_ora.tm_hour > ORA_CHIUSURA || t_ora.tm_hour < ora_oggi->tm_hour){
        return false;
    }else{
        return true;
    } 


}