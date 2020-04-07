CC=gcc

all: serveur client
  

serveur: serveurchat.c
	$(CC) -o serveurchat serveurchat.c

client: clientchat.c
	$(CC) -o clientchat clientchat.c

clean:
	rm -f clientchat serveurchat

