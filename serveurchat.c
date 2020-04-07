//
//
//
//  Created by Océane Staron on 01/12/2018.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <pthread.h>
#include <ctype.h>
#include <signal.h>


#define NB_MAX_CLIENTS 100
#define TAILLE_BUFFER 256

#define BUFFER_SIZE 256

//struct du client
typedef struct{
    int file_descriptor_client;
    char pseudo[11];
    int en_chat;
} client_t;


int creer_socket_tcp_ip();

int affiche_adresse_socket(int sock);

void *traiter_client(void *arg);
int envoyer_message_a_tous(char* expediteur ,char* message); //non utilisee ici
int valider_pseudo_alphanum(char* pseudo, int longueur_pseudo);

void supprimer_client(int idx_client);

void ajouter_client(client_t *nouveau_client);

//tableau contenant toutes les structures de tous les clients
client_t *tab_client[NB_MAX_CLIENTS];

void afficher_tab_client();

int deux_clients_en_attente(int id_clients_en_attente[2]);

void chat_prive(int clients_en_attente[2]);

void on_ferme(int sig);

void nb_connexion(int sig);

int socket_serveur;

int main(){
    printf("Bonjour, je suis un serveur de chat ! Utilisez nc ou clientchat pour vous connecter.\n");

    int connexion_client;
    struct sockaddr_in adresse_client;
    socklen_t longueur_client;
    
    struct ifaddrs *list_itf_reseau; //liste chaînée
    struct ifaddrs *case_courante_list;
    
    char buffer_message[TAILLE_BUFFER];
    
    longueur_client = sizeof(adresse_client);
    
    pthread_t thread_id;
    
    printf("PID : %d\n",(int)getpid());
    
    socket_serveur = creer_socket_tcp_ip();
    
    printf("socket ID : %d\n",socket_serveur);
    
    affiche_adresse_socket(socket_serveur);
    
    getifaddrs(&list_itf_reseau);
    case_courante_list = list_itf_reseau;
    
    while (case_courante_list)
    {
        if (case_courante_list->ifa_addr && case_courante_list->ifa_addr->sa_family == AF_INET) //on ne regarde que les protocoles TCP
        {
            struct sockaddr_in *pAddr = (struct sockaddr_in *)case_courante_list->ifa_addr;
            printf("%s: %s\n", case_courante_list->ifa_name, inet_ntoa(pAddr->sin_addr));
        }
        
        case_courante_list = case_courante_list->ifa_next; //on passe à la case suivante
    }
    
    freeifaddrs(list_itf_reseau);  //on libère la mémoire

    if( listen( socket_serveur, 1 ) < 0){
        perror("listen a foiré. \n");
    }
    
    signal(SIGUSR1, on_ferme);
    signal(SIGUSR2, nb_connexion);
    
    while(1){
        connexion_client = accept(socket_serveur, (struct sockaddr*)&adresse_client, &longueur_client);
        strcpy(buffer_message, "Bienvenue sur le chat !\n");
        
        if (write(connexion_client, buffer_message, strlen(buffer_message)+1) <0){
            perror ("erreur 1");
        }
        
        bzero(buffer_message,256);

        if( pthread_create( &thread_id, NULL, &traiter_client, (void *) &connexion_client ) != 0 ) {
            perror( "Erreur de creation de thread");
            exit(1);
        }
    }
}

int creer_socket_tcp_ip(){
    
    int sock;
    
    struct sockaddr_in adresse;
    
    sock = socket(AF_INET, SOCK_STREAM, 0);
    
    if(sock<0){
        fprintf(stderr, "Erreur socket.\n");
        exit(1);
    }
    
    //initialisation à 0 de tous les bits (sizeof) du bloc mémoire pointé par adresse
    memset(&adresse, 0, sizeof(struct sockaddr_in));
    
    adresse.sin_family = AF_INET; //Protocol IPv4
    adresse.sin_port = htons(33016); //Port (numéro arbitraire, juste lointain)
    adresse.sin_addr.s_addr = htonl(INADDR_ANY); //on veut une adresse dispo sur le reseau, pas en machine locale
    //sock affectée à la struct adresse. Cast : bind veut struct sockaddr
    if (bind(sock, (struct sockaddr*) &adresse, sizeof(struct sockaddr_in)) < 0)
    {
        close(sock);
        perror("Erreur bind");
        exit(2);
    }
    return sock;
}

int affiche_adresse_socket(int sock)
{
    struct sockaddr_in adresse;
    socklen_t longueur;
    longueur = sizeof(struct sockaddr_in);
    if (getsockname(sock, (struct sockaddr*)&adresse, &longueur) < 0)
    {
        close(sock);
        perror("Erreur getsockname");
        exit(3);
    }
    printf("IP = %s, Port = %u\n", inet_ntoa(adresse.sin_addr), ntohs(adresse.sin_port));
    return 0;
}

void *traiter_client(void *arg){
    
    int file_descriptor_client = * ( (int*)arg ); //je n'ai pas trouve plus elegant
    char buffer_message_in[TAILLE_BUFFER];
    char buffer_message_out[TAILLE_BUFFER];
    char msg_attente[TAILLE_BUFFER];
    
    client_t *client = (client_t *)malloc(sizeof(client_t));

    int lecture;
    int fd_clients_en_attente[2] = {0,0};
    
    client->file_descriptor_client = file_descriptor_client;
    client->en_chat = 0;
    
    strcpy(buffer_message_out, "Choisissez votre pseudo \n");
    
    write(file_descriptor_client, buffer_message_out, strlen(buffer_message_out)+1);
    
    while((lecture = read(file_descriptor_client, buffer_message_in, sizeof(buffer_message_in)-1)) < 4 || lecture > 11 || !valider_pseudo_alphanum(buffer_message_in,lecture)){
        
        buffer_message_in[lecture - 1] = '\0';
        strcpy(buffer_message_out, "Entre 3 et 10 caractères, avec des chiffres ou des lettres.\n");
        
        write(file_descriptor_client, buffer_message_out, strlen(buffer_message_out)+1);
        
        bzero(buffer_message_out,256);
        bzero(buffer_message_in,256);
    }
          
    buffer_message_in[lecture - 1]= '\0';
    sprintf(client->pseudo,"%s", buffer_message_in);
    ajouter_client(client);

    bzero(buffer_message_in,256);

    strcpy(msg_attente, "En recherche d'un correspondant...\n");
    
    write(file_descriptor_client, msg_attente,strlen(msg_attente)+1);
    
    bzero(msg_attente,256);

    if(deux_clients_en_attente(fd_clients_en_attente)==1){
        chat_prive(fd_clients_en_attente);
    }
    return NULL;
}


void ajouter_client(client_t *nouveau_client){
    int i;
    
    for(i = 0; i < NB_MAX_CLIENTS; i++){
        if(tab_client[i] == NULL){
            tab_client[i] = nouveau_client;
            return;
        }
    }
}

void supprimer_client(int idx_client){
  
    int socket_client;
    
    socket_client = tab_client[idx_client]->file_descriptor_client;
    close(socket_client);
    tab_client[idx_client] = NULL;
}

int valider_pseudo_alphanum(char* pseudo, int longueur_pseudo){
    
    int i;
    
    for(i = 0; i < longueur_pseudo - 1; i++){
        if(!isalnum(pseudo[i])){
            return 0;
        }
    }
    return 1;
}

void afficher_tab_client(){
    
    int i;
    
    for(i = 0; i < NB_MAX_CLIENTS; i++){
        if(tab_client[i] != NULL){
            printf("i= %d, FD = %d,  pseudo = %s, en chat = %d \n", i, tab_client[i]->file_descriptor_client, tab_client[i]->pseudo, tab_client[i]->en_chat);
        }
    }
}

int deux_clients_en_attente(int fd_clients_en_attente[2]){
    int i;
    int fd1;
    int fd2;
    int nb = 0; // nombre de client en attente trouves
    
    for(i = 0; i < NB_MAX_CLIENTS; i++){
        if(tab_client[i] != NULL){
            if(tab_client[i]->en_chat == 0){
                if (nb == 0){
                    fd1=i;
                    nb =nb+1;
                }else{
                    fd2=i;
                    nb=nb+1;
                    break;
                }
            }
        }
    }
    
    if(nb == 2){
        fd_clients_en_attente[0]=fd1;
        fd_clients_en_attente[1]=fd2;
        return 1;
    }
    return 0;
}

int envoyer_message_a_tous(char* expediteur ,char* message){
    char buffer[TAILLE_BUFFER];
    int i;
    
    strcpy(buffer, expediteur);
    strcat(buffer, " : ");
    strcat(buffer, message);
    
    for(i=0; i<NB_MAX_CLIENTS; i++){
        if(tab_client[i] != NULL){
            write(tab_client[i]->file_descriptor_client, buffer, strlen(buffer)+1);
        }
    }
    return 1;
}
          

void chat_prive(int clients_en_attente[2]){
    
    int i;
    
    int idx1 = clients_en_attente[0];
    int idx2 = clients_en_attente[1];
    
    int fd_clients_en_attente[2] = {0,0};
    
    int numero_tour = 0;
    char buffer_message_in[TAILLE_BUFFER];
    char buffer_message_out[TAILLE_BUFFER];
    
    char ecrit[TAILLE_BUFFER];
    char attend[TAILLE_BUFFER];
    
    int lecture;
    
    char message_accueil[TAILLE_BUFFER];

    tab_client[idx1]->en_chat = 1;
    tab_client[idx2]->en_chat = 1;

    strcpy(message_accueil, "Vous parlez avec : ");

    write(tab_client[idx1]->file_descriptor_client,message_accueil, strlen(message_accueil)+1);
    
    strcpy(message_accueil, "Vous parlez avec : ");

    write(tab_client[idx2]->file_descriptor_client,message_accueil, strlen(message_accueil)+1);

    write(tab_client[idx1]->file_descriptor_client,tab_client[idx2]->pseudo, strlen(tab_client[idx2]->pseudo)+1);
    
    write(tab_client[idx2]->file_descriptor_client,tab_client[idx1]->pseudo, strlen(tab_client[idx1]->pseudo)+1);
    
    bzero(message_accueil,256);

    while(1){
        if(numero_tour%2 == 0){
            
            strcpy(ecrit, "\nEcrivez (exit pour quitter) : \n");
            
            if( write(tab_client[idx1]->file_descriptor_client, ecrit, strlen(ecrit)+1) < 0){
                perror("erreur\n");
            }
            
            bzero(ecrit,256);

            strcpy(attend, "\nAttendez...\n");
            
            write(tab_client[idx2]->file_descriptor_client, attend, strlen(attend)+1);
            bzero(attend,256);

            lecture = read(tab_client[idx1]->file_descriptor_client, buffer_message_in, TAILLE_BUFFER);
            if( lecture == 0){
                break;
            }
            
            buffer_message_in[lecture-1] = '\0';
            
            if ( !strcmp( buffer_message_in, "exit" ) ){
                
                write(tab_client[idx2]->file_descriptor_client,tab_client[idx1]->pseudo, strlen(tab_client[idx1]->pseudo) + 1);
                
                strcpy(message_accueil, " a quitté ! On vous cherche un nouveau correspondant...\n ");
                
                write(tab_client[idx2]->file_descriptor_client,message_accueil, strlen(message_accueil)+1);
                
                supprimer_client(idx1);

                tab_client[idx2]->en_chat = 0;
            
                if(deux_clients_en_attente(fd_clients_en_attente)==1){
                    
                    chat_prive(fd_clients_en_attente);
                }
                break;
            }
            
            write(tab_client[idx2]->file_descriptor_client, buffer_message_in, strlen(buffer_message_in) + 1);
                numero_tour =  numero_tour + 1;
        } else {
                strcpy(ecrit, "\nEcrivez (exit pour quitter): \n");
                write(tab_client[idx2]->file_descriptor_client, ecrit, strlen(ecrit)+1);
                bzero(ecrit,256);

                strcpy(attend, "\nAttendez...\n");
                write(tab_client[idx1]->file_descriptor_client, attend, strlen(attend)+1);
                bzero(attend,256);

                lecture = read(tab_client[idx2]->file_descriptor_client, buffer_message_out, TAILLE_BUFFER);
                if (lecture == 0){
                    break;
                }
                buffer_message_out[lecture-1] = '\0';
                
                if ( !strcmp( buffer_message_out, "exit" ) ){
                    write(tab_client[idx1]->file_descriptor_client,tab_client[idx2]->pseudo, strlen(tab_client[idx2]->pseudo)+1);

                    strcpy(message_accueil, " a quitté ! On vous cherche un nouveau correspondant...\n ");
                    
                    write(tab_client[idx1]->file_descriptor_client,message_accueil, strlen(message_accueil)+1);
                    bzero(message_accueil,256);
                    
                    supprimer_client(idx2);

                    tab_client[idx1]->en_chat =0;
                    
                    if(deux_clients_en_attente(fd_clients_en_attente)==1){
                        chat_prive(fd_clients_en_attente);
                    }
                    break;
                }
                    
                write(tab_client[idx1]->file_descriptor_client, buffer_message_out, strlen(buffer_message_out)+1);
                bzero(buffer_message_out,256);
                numero_tour =  numero_tour + 1;
        }
    }
}

void on_ferme(int sig){
    
    int i;
    char message[TAILLE_BUFFER];
    strcpy(message, "\n On ferme !\n");
    
    for(i = 0; i < NB_MAX_CLIENTS; i++){
        if(tab_client[i] != NULL){
            write(tab_client[i]->file_descriptor_client, message, strlen(message)+1);
            supprimer_client(i);
        }
    }
    if(close(socket_serveur)<0){
        perror("Erreur socket");
    }
    exit(0);
}

    
void nb_connexion(int sig){
    int nb =0;
    int i;
    
    for(i = 0; i < NB_MAX_CLIENTS; i++){
        if(tab_client[i] != NULL){
            nb = nb+1;
        }
    }
    afficher_tab_client();
    printf("Nombre de connexions : %d \n", nb);
}
