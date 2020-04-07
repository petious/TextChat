//
//
//
//  Created by Océane Staron on 04/12/2018.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// en-tete unix:
#include <unistd.h>
// en-tetes reseaux:
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#define BUFFER_SIZE 256

int cree_socket_tcp_ip_client();
void *ecouter_serveur(void *arg);

int main(int argc, char *argv[] ) {
    int port;
    int socket;
    int lecture;
    pthread_t thread_id;
    
    char buffer[BUFFER_SIZE];
    
    if ( argc!= 3 ) {
        printf("usage: connect num_IP num_port.\n");
        exit(1);
    }else{
        // conversion chaine de caractere -> nombre entier:
        sscanf( argv[2], "%d", &port );
        // creation socket de connexion:
        socket = cree_socket_tcp_ip_client( argv[1], port );
    }
    
    
    if ( pthread_create(&thread_id, NULL, &ecouter_serveur,(void *) &socket) != 0 ) {
        perror( "Erreur de creation de thread");
        exit(1);
    }
    
    while(1){
        fgets( buffer, BUFFER_SIZE, stdin );
        write( socket, buffer, strlen(buffer) );
    }
}

void *ecouter_serveur(void *arg){
    int lecture;
    char buffer[BUFFER_SIZE];
    int socket;
    char buffer2[BUFFER_SIZE];
    char buffer3[BUFFER_SIZE];

    int i;
    int temp, lol;
    
    socket = *((int *) arg);
    bzero(buffer,256);

    while((lecture =  read(socket, buffer, BUFFER_SIZE)) > 0){
        buffer[lecture-1] = '\0';
        printf("%s\n",buffer);
        
        //si plusieurs messages sont contenus dans le buffer. la variable temp sert à dépiler lecture
        if(lecture-1 > (int)strlen(buffer)){
            temp = lecture-1-(int)strlen(buffer);
            for(i=0; i<lecture-1-(int)strlen(buffer); i++){
                buffer2[i]=buffer[(int)strlen(buffer)+1 +i];
            }
            printf("%s\n", buffer2);
            temp = temp-(int)strlen(buffer2)-1;
            
            if(lecture - 1 > (int)strlen(buffer) + (int)strlen(buffer2) && temp > 0){
                for(i=0; i < lecture - 1 - (int)strlen(buffer2); i++){
                    buffer3[i] = buffer2[(int)strlen(buffer2) + 1 + i];
                }
                printf("%s\n", buffer3);
            }
        }
        bzero(buffer,256);
        fflush(stdout);
    }
    return NULL;
}

int cree_socket_tcp_ip_client( char *ip, int port ) {
    int socket_contact;
    struct sockaddr_in adresse;
    // IP et TCP:
    if ((socket_contact = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Erreur socket");
        exit(1);
    }
    // initialisation a 0 puis affectation des champs:
    memset(&adresse, 0, sizeof(struct sockaddr_in));
    adresse.sin_family = AF_INET; // Protocol IP
    adresse.sin_port = htons(port); // port 33016
    inet_aton(ip, &adresse.sin_addr); // IP internet
    
    connect(socket_contact, (struct sockaddr*) &adresse, sizeof(struct sockaddr_in));
 
    return socket_contact;
}

