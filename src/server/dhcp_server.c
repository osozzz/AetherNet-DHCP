#include "dhcp_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>

#define DHCP_DISCOVER 1
#define DHCP_OFFER 2
#define DHCP_REQUEST 3
#define DHCP_DECLINE 4
#define DHCP_ACK 5
#define DHCP_NACK 6
#define DHCP_RELEASE 7

// Estructura básica del paquete DHCP (simplificado)

// Lista de asignaciones de IP
ip_assignment_t assigned_ips[MAX_IPS];
int current_ip_index = 0;

// Función para inicializar el servidor DHCP
void init_dhcp_server() {
    int sockfd;
    struct sockaddr_in server_addr;

    // Crear el socket UDP
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Error al crear el socket");
        exit(EXIT_FAILURE);
    }
    printf("Socket creado exitosamente.\n");

    // Configurar la dirección del servidor
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // Escuchar en todas las interfaces
    server_addr.sin_port = htons(DHCP_SERVER_PORT);  // Puerto 67

    // Vincular el socket al puerto
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error al vincular el socket");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    printf("Socket vinculado exitosamente al puerto %d.\n", DHCP_SERVER_PORT);

    // Llamar a la función para manejar solicitudes DHCP
    handle_dhcp_requests(sockfd);
    
    close(sockfd);
}

// Función para manejar las solicitudes DHCP entrantes
void handle_dhcp_requests(int sockfd) {
    struct sockaddr_in client_addr;
    char buffer[BUFFER_SIZE];
    socklen_t client_addr_len = sizeof(client_addr);
    struct dhcp_packet *request;

    ip_range_t ip_range;
    initialize_ip_range(&ip_range, "192.168.1.100", "192.168.1.150", 86400); // Rango de IPs y lease time de 24h

    // Bucle infinito para escuchar solicitudes
    while (1) {
        printf("Esperando solicitudes DHCP...\n");

        // Recibir un mensaje del cliente
        ssize_t received_len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, &client_addr_len);
        if (received_len < 0) {
            perror("Error al recibir el mensaje");
            continue;
        }

        // Interpretar el paquete como un paquete DHCP
        request = (struct dhcp_packet *)buffer;

        // Verificar si es un mensaje DHCP Discover
        if (request->op == DHCP_DISCOVER) {
            printf("DHCP Discover recibido de %s\n", inet_ntoa(client_addr.sin_addr));
            send_dhcp_offer(sockfd, request, &client_addr, &ip_range);
        } else if (request->op == DHCP_REQUEST) {
            printf("DHCP Request recibido de %s\n", inet_ntoa(client_addr.sin_addr));
            handle_dhcp_request(sockfd, request, &client_addr, &ip_range);
        } else if (request->op == DHCP_RELEASE) {
            printf("DHCP Release recibido de %s\n", inet_ntoa(client_addr.sin_addr));
            handle_dhcp_release(sockfd, request, &ip_range);
        } else if (request->op == DHCP_DECLINE) {
            printf("DHCP Decline recibido de %s\n", inet_ntoa(client_addr.sin_addr));
            handle_dhcp_decline(sockfd, request, &ip_range);
        } else {
            printf("Mensaje DHCP no reconocido.\n");
        }
    }
}

// Función para enviar un mensaje DHCP Offer
void send_dhcp_offer(int sockfd, struct dhcp_packet *request, struct sockaddr_in *client_addr, ip_range_t *ip_range) {
    struct dhcp_packet response;
    memset(&response, 0, sizeof(response));

    // Rellenar los campos básicos del paquete DHCP Offer
    response.op = 2; // Respuesta del servidor
    response.htype = 1; // Tipo de hardware (Ethernet)
    response.hlen = 6; // Longitud de la dirección MAC
    response.xid = request->xid; // Copiamos el Transaction ID del cliente
    memcpy(response.chaddr, request->chaddr, 16); // Copiamos la dirección MAC del cliente

    // Asignar una dirección IP desde el rango
    char* offered_ip = assign_ip_address(ip_range);
    if (offered_ip == NULL) {
        printf("No hay más direcciones IP disponibles.\n");
        return;
    }
    response.yiaddr = inet_addr(offered_ip); // Dirección IP ofrecida
    printf("Ofreciendo IP: %s\n", offered_ip);

    // Enviar el mensaje de respuesta
    if (sendto(sockfd, &response, sizeof(response), 0, (struct sockaddr *)client_addr, sizeof(*client_addr)) < 0) {
        perror("Error al enviar el DHCP Offer");
    } else {
        printf("DHCP Offer enviado a %s\n", inet_ntoa(client_addr->sin_addr));
    }
}

// Función para inicializar el rango de direcciones IP
void initialize_ip_range(ip_range_t *ip_range, const char* start_ip, const char* end_ip, int lease_time) {
    ip_range->start_ip = strdup(start_ip);
    ip_range->end_ip = strdup(end_ip);
    ip_range->lease_time = lease_time;
}

// Función para asignar una dirección IP dinámica del rango
char* assign_ip_address(ip_range_t *ip_range) {
    static int next_ip = 100; // Simulando IPs desde 192.168.1.100 a 192.168.1.150
    char *assigned_ip = (char *)malloc(16);

    if (next_ip > 150) {
        return NULL; // No más IPs disponibles
    }

    sprintf(assigned_ip, "192.168.1.%d", next_ip);
    next_ip++;
    return assigned_ip;
}

void send_dhcp_ack(int sockfd, struct dhcp_packet *request, struct sockaddr_in *client_addr, ip_range_t *ip_range) {
    struct dhcp_packet response;
    memset(&response, 0, sizeof(response));

    // Configurar el mensaje DHCP ACK
    response.op = 2; // Respuesta del servidor
    response.htype = 1; // Tipo de hardware (Ethernet)
    response.hlen = 6; // Longitud de la dirección MAC
    response.xid = request->xid; // Copiamos el Transaction ID del cliente
    memcpy(response.chaddr, request->chaddr, 16); // Copiamos la dirección MAC del cliente

    // Asignar la dirección IP previamente ofrecida (yiaddr del DHCPOFFER)
    response.yiaddr = request->yiaddr;

    // Configurar las opciones adicionales (máscara de subred, gateway, DNS)
    response.options[0] = 1; // Máscara de subred
    response.options[1] = 4; // Longitud de la opción
    response.options[2] = 5; response.options[3] = 255;
    response.options[4] = 255; response.options[5] = 255;
    response.options[6] = 0; // 255.255.255.0

    response.options[7] = 3; // Gateway
    response.options[8] = 4;
    response.options[9] = 192; response.options[10] = 168;
    response.options[11] = 1; response.options[12] = 1; // 192.168.1.1

    // Enviar el mensaje DHCP ACK
    if (sendto(sockfd, &response, sizeof(response), 0, (struct sockaddr *)client_addr, sizeof(*client_addr)) < 0) {
        perror("Error al enviar el DHCP ACK");
    } else {
        printf("DHCP ACK enviado a %s\n", inet_ntoa(client_addr->sin_addr));
    }
}

void send_dhcp_nak(int sockfd, struct dhcp_packet *request, struct sockaddr_in *client_addr) {
    struct dhcp_packet response;
    memset(&response, 0, sizeof(response));

    // Configurar el mensaje DHCPNAK
    response.op = 2; // Respuesta del servidor
    response.htype = 1; // Tipo de hardware (Ethernet)
    response.hlen = 6; // Longitud de la dirección MAC
    response.xid = request->xid; // Copiamos el Transaction ID del cliente
    memcpy(response.chaddr, request->chaddr, 16); // Copiamos la dirección MAC del cliente

    // Opciones DHCP (53 = DHCP Message Type, 6 = DHCPNAK)
    response.options[0] = 53;
    response.options[1] = 1;
    response.options[2] = 6;  // DHCPNAK

    // Enviar el mensaje DHCPNAK
    if (sendto(sockfd, &response, sizeof(response), 0, (struct sockaddr *)client_addr, sizeof(*client_addr)) < 0) {
        perror("Error al enviar el DHCPNAK");
    } else {
        printf("DHCPNAK enviado a %s\n", inet_ntoa(client_addr->sin_addr));
    }
}

void handle_dhcp_request(int sockfd, struct dhcp_packet *request, struct sockaddr_in *client_addr, ip_range_t *ip_range) {
    // Verificar si la IP solicitada es válida
    if (!is_ip_valid(request->yiaddr, ip_range)) {
        // Enviar DHCPNAK si la IP no es válida
        send_dhcp_nak(sockfd, request, client_addr);
    } else {
        // Si es válida, enviar el DHCPACK
        send_dhcp_ack(sockfd, request, client_addr, ip_range);
    }
}

void handle_dhcp_decline(int sockfd, struct dhcp_packet *decline_message, ip_range_t *ip_range) {
    // Convertir la IP en yiaddr a una cadena legible
    struct in_addr declined_ip;
    declined_ip.s_addr = decline_message->yiaddr;

    printf("DHCPDECLINE recibido para la IP: %s\n", inet_ntoa(declined_ip));

    // Marcar la IP como no disponible
    mark_ip_as_unavailable(ip_range, decline_message->yiaddr);
}

void handle_dhcp_release(int sockfd, struct dhcp_packet *release_message, ip_range_t *ip_range) {
    // Convertir la IP en ciaddr (dirección IP del cliente) a una cadena legible
    struct in_addr released_ip;
    released_ip.s_addr = release_message->ciaddr;

    printf("DHCPRELEASE recibido para la IP: %s\n", inet_ntoa(released_ip));

    // Marcar la IP como disponible nuevamente
    mark_ip_as_available(ip_range, release_message->ciaddr);
}

void mark_ip_as_unavailable(ip_range_t *ip_range, uint32_t ip) {
    // Lógica para marcar la IP como no disponible en el rango
    // (Actualizar la tabla de asignaciones IP)
    printf("IP %s marcada como no disponible.\n", inet_ntoa(*(struct in_addr *)&ip));
}

void mark_ip_as_available(ip_range_t *ip_range, uint32_t ip) {
    // Lógica para marcar la IP como disponible en el rango
    printf("IP %s marcada como disponible.\n", inet_ntoa(*(struct in_addr *)&ip));
}

int is_ip_valid(uint32_t ip, ip_range_t *ip_range) {
    // Verificar si la IP está dentro del rango
    uint32_t start_ip = inet_addr(ip_range->start_ip);
    uint32_t end_ip = inet_addr(ip_range->end_ip);

    return (ip >= start_ip && ip <= end_ip);
}