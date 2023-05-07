# make rule primaria con dummy target 'all'

all: cli server td kd


#make rule per il server
server: server.o
	gcc -Wall server.o -o server 

server.o: server.c
	gcc -c server.c


#make rule per il client
cli: client.o
	gcc -Wall client.o -o cli 

client.o: client.c 
	gcc -c client.c


#make rule per il kitchen device
kd: kd.o
	gcc -Wall kd.o -o kd 

kd.o: kd.c
	gcc -c kd.c
#meke rule per il table device
td:td.o
	gcc -Wall td.o -o td 

td.o:td.c 
	gcc -c td.c

# pulizia dei file della compilazione
clean:
	rm *o cli server td kd