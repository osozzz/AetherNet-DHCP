#include "dhcp_client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define DHCP_SERVER_PORT 67
#define DHCP_CLIENT_PORT 68

void start_dhcp_client() {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    int broadcastEnable = 1; 
    
    // Crear socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Error al crear el socket");
        exit(EXIT_FAILURE);
    }

     // Habilitar broadcast en el socket
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable)) < 0) {
        perror("Error al habilitar broadcast");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Configurar dirección del cliente (puerto 68)
    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(DHCP_CLIENT_PORT);
    client_addr.sin_addr.s_addr = INADDR_ANY;

    // Enlazar el socket a la dirección del cliente
    if (bind(sockfd, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) {
        perror("Error al enlazar el socket");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Configurar dirección del servidor (puerto 67)
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(DHCP_SERVER_PORT);
    server_addr.sin_addr.s_addr = INADDR_BROADCAST;

    // Enviar mensaje DHCPDISCOVER
    send_dhcp_discover(sockfd, &server_addr);

    // Recibir respuesta (DHCP Offer)
    receive_dhcp_offer(sockfd);

    // Cerrar el socket al finalizar
    close(sockfd);
}

void send_dhcp_discover(int sockfd, struct sockaddr_in *server_addr) {
    struct dhcp_packet discover_message;

    // Inicializar el mensaje DHCPDISCOVER
    memset(&discover_message, 0, sizeof(discover_message)); // Limpiar la estructura
    discover_message.op = 1;  // Solicitud (1 para cliente)
    discover_message.htype = 1;  // Tipo de hardware: Ethernet
    discover_message.hlen = 6;  // Longitud de hardware: 6 para dirección MAC
    discover_message.xid = htonl(0x3903F326);  // ID de transacción (puede ser aleatorio)
    
    // Configurar la dirección MAC del cliente
    discover_message.chaddr[0] = 0x00;
    discover_message.chaddr[1] = 0x1A;
    discover_message.chaddr[2] = 0x2B;
    discover_message.chaddr[3] = 0x3C;
    discover_message.chaddr[4] = 0x4D;
    discover_message.chaddr[5] = 0x5E;

    // Opciones DHCP (53 = DHCP Message Type, 1 = DHCPDISCOVER)
    discover_message.options[0] = 53;
    discover_message.options[1] = 1;
    discover_message.options[2] = DHCP_DISCOVER;  // DHCPDISCOVER

    // Enviar el mensaje al servidor DHCP
    int send_len = sendto(sockfd, &discover_message, sizeof(discover_message), 0,
                          (struct sockaddr *)server_addr, sizeof(*server_addr));
    if (send_len < 0) {
        perror("Error al enviar el mensaje DHCPDISCOVER");
    } else {
        printf("Mensaje DHCPDISCOVER enviado.\n");
    }
}

void receive_dhcp_offer(int sockfd) {
    char offer_message[BUFFER_SIZE];
    struct sockaddr_in server_addr;
    socklen_t addr_len = sizeof(server_addr);

    // Recibir el mensaje DHCP Offer del servidor
    int recv_len = recvfrom(sockfd, offer_message, sizeof(offer_message), 0,
                            (struct sockaddr *)&server_addr, &addr_len);
    if (recv_len < 0) {
        perror("Error al recibir el mensaje DHCPOFFER");
    } else {
        printf("Mensaje DHCPOFFER recibido.\n");
    }

    // Procesa el mensaje DHCP Offer aquí (extraer dirección IP, máscara, etc.)
}
