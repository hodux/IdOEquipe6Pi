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
#define PORT 8888
#define DEST_IP "127.0.0.1"

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
    printf("SENT - %s\n", statusData);
}

int main() {
    int handle;

    // Initialiser pigpio
    if (gpioInitialise() < 0) {
        printf("Erreur d'initialisation pigpio\n");
        return 1;
    }

    ///////// BTN /////////
    sendStatus();

    ///////// SPLIT team:status /////////
    char str[] = "1:1";

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

    printf("RECEIVED - TEAM: %s STATUS: %s\n", team, status);

    ///////// MATRIX /////////
    // Récupérer le référence ("handle") de la matrice
    handle = i2cOpen(1, ADR_I2C, 0);
    
    if (handle < 0) {
        printf("Erreur d'accès à la matrice LED\n");
        gpioTerminate();
        return 1;
    }

    // Initialiser la matrice
    i2cWriteByteData(handle, ADR_SYS_MATRICE | 1, 0x00);
    i2cWriteByteData(handle, ADR_AFFICHAGE_MATRICE | 1, 0x00);

    // Tout éteindre
    for (int i=0;i<0x0E;i+=2) {
        i2cWriteByteData(handle, i, 0x00);
    }

    unsigned char binaryData = 0;
    


    printf("%d\n", binaryData);

    // Allumer puis éteindre la 1ère rangée (0)
    i2cWriteByteData(handle, 0x0e, 0xFF);
    time_sleep(1);
    i2cWriteByteData(handle, 0x0e, 0x00);

    // Allumer puis éteindre la dernière rangée (14 ou 0e)
    // i2cWriteByteData(handle, 0x0e, 0xFF);
    // time_sleep(1);
    // i2cWriteByteData(handle, 0x0e, 0x00);

    // Allumer puis éteindre les 4 LED des coins (10000001 = 128 = 0x81)
    // i2cWriteByteData(handle, 0x00, 0x81);
    // i2cWriteByteData(handle, 0x0e, 0x81);
    // time_sleep(1);
    // i2cWriteByteData(handle, 0x00, 0x00);
    // i2cWriteByteData(handle, 0x0e, 0x00);


    // Fermer et terminer
    i2cClose(handle);
    gpioTerminate();

    return 0;
}
