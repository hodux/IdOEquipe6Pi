#include <stdio.h>
#include <pigpio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// Gpio
#define BTN_PIN 17
// Adresses 
#define ADR_I2C 0x70
#define ADR_SYS_MATRICE 0x20
#define ADR_AFFICHAGE_MATRICE 0x80
// Tcp
#define BUFFER_SIZE 1024
#define SEND_PORT 8888
#define RECEIVE_PORT 9090
#define DEST_IP1 "10.10.0.197"
#define DEST_IP2 "10.10.20.247"

int teams[8];
int states[8];

void storeTeamStates(char str[]) {
    char *token;
    char team[2];
    char status[2];
    int teamIndex;

    token = strtok(str, ":");
    if (token != NULL) {
        strcpy(team, token);
        teamIndex = atoi(team) -1;
    }

    token = strtok(NULL, ":");
    if (token != NULL) {
        strcpy(status, token);
    }

    if (teamIndex >= 0 && teamIndex <= 7) {
        teams[teamIndex] = teamIndex + 1;
        states[teamIndex] = atoi(status);
    }

    printf("RECEIVED - %d:%d\n", teamIndex + 1, atoi(status));
}

unsigned char getMatrixState(int teams[], int states[]) {
    unsigned char matrixState = 0;
    
    for (int i = 0; i < 8; i++) {
        if (states[i] == 1) {
            matrixState |= (1 << (teams[i] - 1));
        }
    }
    
    return matrixState;
}

void *sendStatus() {
    int sock = 0;
    struct sockaddr_in dest_addr;
    // Créer le socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    // Initialiser la struct de l'adresse IP 
    memset(&dest_addr, '0', sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = inet_addr(DEST_IP2);
    dest_addr.sin_port = htons(SEND_PORT);
    // Créer la connexion
    connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));

    gpioSetMode(BTN_PIN, PI_INPUT);
    // Envoyer le message
    while (1) {
        int myStatus = gpioRead(BTN_PIN);
        myStatus = (myStatus == 0) ? 1 : 0;
        
        char myTeam[] = "6";
        char statusData[5];
        snprintf(statusData,sizeof(statusData), "%s:%d", myTeam, myStatus);
        send(sock, statusData, strlen(statusData), 0);
        printf("SENT - %s TO %s\n\r", statusData, DEST_IP2);
        sleep(1);
    }

    close(sock);
}

void *receiveStatus() {
    int socket_local, socket_dist;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};

    // Créer le socket et initialiser l'adresse
    socket_local = socket(AF_INET, SOCK_STREAM, 0);
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(RECEIVE_PORT);

    // Associer le socket à l'adresse de l'interface
    bind(socket_local, (struct sockaddr *)&address, sizeof(address));

    // Attendre une connexion entrante
    printf("- Waiting for connexion -\n");
    listen(socket_local, 3);
    socket_dist = accept(socket_local, (struct sockaddr *)&address, (socklen_t*)&addrlen);
    printf("- Connected -\n");

    int handle;
    // Récupérer le référence ("handle") de la matrice
    handle = i2cOpen(1, ADR_I2C, 0);
    // Initialiser la matrice
    i2cWriteByteData(handle, ADR_SYS_MATRICE | 1, 0x00);
    i2cWriteByteData(handle, ADR_AFFICHAGE_MATRICE | 1, 0x00);
    // Tout éteindre
    for (int i=0;i<0x0e;i+=2) {
        i2cWriteByteData(handle, i, 0x00);
    }

    // Réception des messages
    while(1) {
        int datalen;
        // Stocker le message
        datalen = read(socket_dist, buffer, BUFFER_SIZE);
        if (datalen == 0) {
            printf("Déconnexion\n");

            break;
        } else { // Afficher le message
            fflush(stdout);

            storeTeamStates(buffer);
            i2cWriteByteData(handle, 0x0e, getMatrixState(teams, states));
            time_sleep(1);
        }
        memset(buffer, 0, BUFFER_SIZE);
    }

    close(socket_dist);
    close(socket_local);
}

int main() {
    // Initialiser pigpio
    if (gpioInitialise() < 0) {
        printf("Erreur d'initialisation pigpio\n");
        return 1;
    }

    ///////// SEND & RECEIVE /////////
    pthread_t send_thread, receive_thread;

    pthread_create(&send_thread, NULL, sendStatus, NULL);
    pthread_create(&receive_thread, NULL, receiveStatus, NULL);

    pthread_join(send_thread, NULL);
    pthread_join(receive_thread, NULL);

    // Fermer et terminer
    gpioTerminate();

    return 0;
}
