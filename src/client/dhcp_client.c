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
    struct in_addr offered_ip;  // Aquí almacenamos la IP ofrecida
    
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
    receive_dhcp_offer(sockfd, &offered_ip);

    // Verificar si la IP ofrecida está en uso
    if (check_ip_conflict(inet_ntoa(offered_ip))) {
        // Si hay conflicto, enviar DHCPDECLINE
        printf("IP ofrecida está en uso. Enviando DHCPDECLINE.\n");
        send_dhcp_decline(sockfd, &offered_ip, &server_addr);
    } else {
        printf("IP ofrecida no está en uso. Enviando DHCPREQUEST.\n");
        // Si no hay conflicto, continuar con DHCPREQUEST
        send_dhcp_request(sockfd, &server_addr, &offered_ip);

        // Recibir confirmación DHCPACK o DHCPNAK
        if (receive_dhcp_ack_or_nak(sockfd) == 0) {
            // Si recibimos DHCPNAK, reiniciar el proceso
            printf("Reiniciando el proceso DHCP...\n");
            close(sockfd);  // Cerrar el socket actual
            start_dhcp_client();  // Volver a iniciar el cliente DHCP
            return;
        }

        // Enviar DHCPRELEASE al finalizar
        send_dhcp_release(sockfd, &server_addr, &offered_ip);
    }

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

void receive_dhcp_offer(int sockfd, struct in_addr *offered_ip) {
    struct dhcp_packet offer_message;
    struct sockaddr_in server_addr;
    socklen_t addr_len = sizeof(server_addr);

    // Recibir el mensaje DHCP Offer del servidor
    int recv_len = recvfrom(sockfd, &offer_message, sizeof(offer_message), 0,
                            (struct sockaddr *)&server_addr, &addr_len);
    if (recv_len < 0) {
        perror("Error al recibir el mensaje DHCPOFFER");
    } else {
        printf("Mensaje DHCPOFFER recibido.\n");

        offered_ip->s_addr = offer_message.yiaddr;  // Asignamos el valor de yiaddr a la estructura in_addr
        printf("Dirección IP ofrecida: %s\n", inet_ntoa(*offered_ip));  // Imprimir la dirección IP ofrecida
    }
}

int check_ip_conflict(const char *ip_address) {
    char command[100];
    snprintf(command, sizeof(command), "ping -c 1 %s > /dev/null", ip_address);
    int ret = system(command);
    return ret == 0;  // Retorna 0 si hay respuesta, indicando conflicto
}

int receive_dhcp_ack_or_nak(int sockfd) {
    struct dhcp_packet response_message;
    struct sockaddr_in server_addr;
    socklen_t addr_len = sizeof(server_addr);

    // Recibir el mensaje DHCPACK o DHCPNAK del servidor
    int recv_len = recvfrom(sockfd, &response_message, sizeof(response_message), 0,
                            (struct sockaddr *)&server_addr, &addr_len);
    if (recv_len < 0) {
        perror("Error al recibir el mensaje DHCPACK o DHCPNAK");
        return 0;
    }

    // Comprobar si es DHCPACK o DHCPNAK
    if (response_message.options[2] == DHCP_ACK) {
        printf("Mensaje DHCPACK recibido.\n");
        printf("Dirección IP asignada: %s\n", inet_ntoa(*(struct in_addr *)&response_message.yiaddr));
        return 1;  // Indica que se recibió DHCPACK
    } else if (response_message.options[2] == DHCP_NAK) {
        printf("Mensaje DHCPNAK recibido. La solicitud fue rechazada.\n");
        return 0;  // Indica que se recibió DHCPNAK
    } else {
        printf("Mensaje desconocido recibido. No es DHCPACK ni DHCPNAK.\n");
        return 0;  // Indica que se recibió un mensaje desconocido
    }

    return 0;  // Si no es ninguno de los dos, retornamos 0 por seguridad
}

void send_dhcp_decline(int sockfd, struct in_addr *offered_ip, struct sockaddr_in *server_addr) {
    struct dhcp_packet decline_message;
    memset(&decline_message, 0, sizeof(decline_message));

    // Configurar el mensaje DHCPDECLINE
    decline_message.op = 1;  // Solicitud del cliente
    decline_message.htype = 1;  // Tipo de hardware: Ethernet
    decline_message.hlen = 6;  // Longitud de la dirección MAC
    decline_message.xid = htonl(0x3903F326);  // Usar el mismo Transaction ID
    memcpy(decline_message.chaddr, "001A2B3C4D5E", 6);  // Dirección MAC del cliente
    decline_message.yiaddr = offered_ip->s_addr;  // IP en conflicto

    // Opciones DHCP (53 = DHCP Message Type, 4 = DHCPDECLINE)
    decline_message.options[0] = 53;
    decline_message.options[1] = 1;
    decline_message.options[2] = 4;  // DHCPDECLINE

    // Enviar el mensaje DHCPDECLINE
    if (sendto(sockfd, &decline_message, sizeof(decline_message), 0, (struct sockaddr *)server_addr, sizeof(*server_addr)) < 0) {
        perror("Error al enviar el mensaje DHCPDECLINE");
    } else {
        printf("Mensaje DHCPDECLINE enviado.\n");
    }
}

void send_dhcp_request(int sockfd, struct sockaddr_in *server_addr, struct in_addr *offered_ip) {
    struct dhcp_packet request_message;

    // Inicializar el mensaje DHCPREQUEST
    memset(&request_message, 0, sizeof(request_message)); // Limpiar la estructura
    request_message.op = 3;  // Solicitud (1 para cliente)
    request_message.htype = 1;  // Tipo de hardware: Ethernet
    request_message.hlen = 6;  // Longitud de hardware: 6 para dirección MAC
    request_message.xid = htonl(0x3903F326);  // Usar el mismo Transaction ID del DHCPDISCOVER
    
    // Configurar la dirección MAC del cliente
    request_message.chaddr[0] = 0x00;
    request_message.chaddr[1] = 0x1A;
    request_message.chaddr[2] = 0x2B;
    request_message.chaddr[3] = 0x3C;
    request_message.chaddr[4] = 0x4D;
    request_message.chaddr[5] = 0x5E;

    // Usar la IP ofrecida (recibida en el DHCPOFFER)
    request_message.yiaddr = offered_ip->s_addr;

    // Opciones DHCP (53 = DHCP Message Type, 3 = DHCPREQUEST)
    request_message.options[0] = 53;
    request_message.options[1] = 3;
    request_message.options[2] = DHCP_REQUEST;  // DHCPREQUEST

    // Enviar el mensaje al servidor DHCP
    int send_len = sendto(sockfd, &request_message, sizeof(request_message), 0,
                          (struct sockaddr *)server_addr, sizeof(*server_addr));
    if (send_len < 0) {
        perror("Error al enviar el mensaje DHCPREQUEST");
    } else {
        printf("Mensaje DHCPREQUEST enviado.\n");
    }
}

void receive_dhcp_ack(int sockfd) {
    struct dhcp_packet ack_message;
    struct sockaddr_in server_addr;
    socklen_t addr_len = sizeof(server_addr);

    // Recibir el mensaje DHCPACK del servidor
    int recv_len = recvfrom(sockfd, &ack_message, sizeof(ack_message), 0,
                            (struct sockaddr *)&server_addr, &addr_len);
    if (recv_len < 0) {
        perror("Error al recibir el mensaje DHCPACK");
    } else {
        printf("Mensaje DHCPACK recibido.\n");
        printf("Dirección IP asignada: %s\n", inet_ntoa(*(struct in_addr *)&ack_message.yiaddr));
    }
}

void send_dhcp_release(int sockfd, struct sockaddr_in *server_addr, struct in_addr *assigned_ip) {
    struct dhcp_packet release_message;
    memset(&release_message, 0, sizeof(release_message));

    // Configurar el mensaje DHCPRELEASE
    release_message.op = 7;  // Solicitud del cliente
    release_message.htype = 1;  // Tipo de hardware: Ethernet
    release_message.hlen = 6;  // Longitud de la dirección MAC
    release_message.xid = htonl(0x3903F326);  // Usamos el Transaction ID del ACK
    memcpy(release_message.chaddr, "001A2B3C4D5E", 6);  // Dirección MAC del cliente
    release_message.ciaddr = assigned_ip->s_addr;  // IP asignada que se va a liberar

    // Opciones DHCP (53 = DHCP Message Type, 7 = DHCPRELEASE)
    release_message.options[0] = 53;
    release_message.options[1] = 1;
    release_message.options[2] = 7;  // DHCPRELEASE

    // Enviar el mensaje DHCPRELEASE
    if (sendto(sockfd, &release_message, sizeof(release_message), 0, (struct sockaddr *)server_addr, sizeof(*server_addr)) < 0) {
        perror("Error al enviar el mensaje DHCPRELEASE");
    } else {
        printf("Mensaje DHCPRELEASE enviado.\n");
    }
}