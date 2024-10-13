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
            
            // Asignar una dirección IP y enviar una oferta
            send_dhcp_offer(sockfd, request, &client_addr, &ip_range);
        } else {
            printf("No es un mensaje DHCP Discover.\n");
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
