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
#define PORT 8888
#define DEST_IP "10.10.0.230"

void sendStatus() {
    int sock = 0;
    struct sockaddr_in dest_addr;
    // Créer le socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    // Initialiser la struct de l'adresse IP 
    memset(&dest_addr, '0', sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = inet_addr(DEST_IP);
    dest_addr.sin_port = htons(PORT);
    // Créer la connexion
    connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));

    gpioSetMode(BTN_PIN, PI_INPUT);
    int myStatus = gpioRead(BTN_PIN);

    // For some reason, have to invert btn status
    myStatus = (myStatus == 0) ? 1 : 0;
    char myTeam[10] = "6";
    char statusData[20];
    snprintf(statusData,sizeof(statusData), "%s:%d", myTeam, myStatus);

    // Envoyer le message et fermer la connexion
    send(sock, statusData, strlen(statusData), 0);
    close(sock);
    printf("SENT - %s\n\r", statusData);
}

char receiveStatus() {
    int socket_local, socket_dist;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};

    // Créer le socket et initialiser l'adresse
    socket_local = socket(AF_INET, SOCK_STREAM, 0);
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Associer le socket à l'adresse de l'interface
    bind(socket_local, (struct sockaddr *)&address, sizeof(address));
    
    // Attendre une connexion entrante
    listen(socket_local, 3);
    socket_dist = accept(socket_local, (struct sockaddr *)&address, (socklen_t*)&addrlen);

    // Réception des messages
    while(1) {
        int datalen;
        // Stocker le message
        datalen = read(socket_dist, buffer, BUFFER_SIZE);
        if (datalen == 0) {
            printf("Déconnexion\n");
            break;
        } else { // Afficher le message
            printf("Reçu: %s", buffer);
            fflush(stdout); 
        }
        memset(buffer, 0, BUFFER_SIZE); 
    }

    close(socket_dist);
    close(socket_local);
    return buffer;    
}

void storeTeamStates(char str[]) {
    char *token;
    char team[10];
    char status[10];
    token = strtok(str, ":");
    if (token != NULL) {
        strcpy(team, token);
    }
    token = strtok(NULL, ":");
    if (token != NULL) {
        strcpy(status, token);
    }
    printf("RECEIVED - %s:%s\n", team, status);
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

int main() {
    // Initialiser pigpio
    // if (gpioInitialise() < 0) {
    //     printf("Erreur d'initialisation pigpio\n");
    //     return 1;
    // }

    ///////// SEND /////////
    // sendStatus();

    ///////// RECEIVE /////////
    // receiveStatus();
    // storeTeamStatus(str);

    int teams[] = {1, 2, 3, 4, 5, 6, 7, 8};
    int states[] = {0, 0, 0, 0, 0, 0, 0, 1}; 
        
    unsigned char matrixState = getMatrixState(teamNumbers, states);
    
    printf("matrixState: %u\n", matrixState);

    ///////// MATRIX /////////
    // int handle;
    // Récupérer le référence ("handle") de la matrice
    // handle = i2cOpen(1, ADR_I2C, 0);
    
    // if (handle < 0) {
    //     printf("Erreur d'accès à la matrice LED\n");
    //     gpioTerminate();
    //     return 1;
    // }

    // // Initialiser la matrice
    // i2cWriteByteData(handle, ADR_SYS_MATRICE | 1, 0x00);
    // i2cWriteByteData(handle, ADR_AFFICHAGE_MATRICE | 1, 0x00);

    // // Tout éteindre
    // for (int i=0;i<0x0E;i+=2) {
    //     i2cWriteByteData(handle, i, 0x00);
    // }

    // i2cWriteByteData(handle, 0x0e, 128);
    // time_sleep(1);
    // i2cWriteByteData(handle, 0x0e, 0x00);

    // // Fermer et terminer
    // i2cClose(handle);
    // gpioTerminate();

    return 0;
}
