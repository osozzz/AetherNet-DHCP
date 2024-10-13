#include "dhcp_client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int main() {
    int sockfd;
    struct sockaddr_in client_addr, server_addr;

    // Crear el socket UDP
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Error al crear el socket");
        exit(EXIT_FAILURE);
    }
    printf("Socket creado exitosamente.\n");

    // Configurar la dirección del cliente (puerto 68)
    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = INADDR_ANY;
    client_addr.sin_port = htons(DHCP_CLIENT_PORT);

    // Vincular el socket al puerto del cliente
    if (bind(sockfd, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) {
        perror("Error al vincular el socket");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    printf("Socket vinculado exitosamente al puerto %d.\n", DHCP_CLIENT_PORT);

    // Configurar la dirección del servidor (broadcast)
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);  // Usar broadcast
    server_addr.sin_port = htons(DHCP_SERVER_PORT);

    // Permitir envío de paquetes broadcast
    int broadcastEnable = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable)) < 0) {
        perror("Error habilitando broadcast");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Enviar mensaje DHCP Discover
    send_dhcp_discover(sockfd, &server_addr);

    // Recibir respuesta DHCP Offer
    receive_dhcp_offer(sockfd);

    // Cerrar el socket
    close(sockfd);

    return 0;
}
