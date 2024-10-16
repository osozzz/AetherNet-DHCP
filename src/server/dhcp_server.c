#include "dhcp_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>
#include <time.h>

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
    initialize_ip_range(&ip_range, "192.168.1.100", "192.168.1.150", 20); // Rango de IPs y lease time de 24h

    // Bucle infinito para escuchar solicitudes
    while (1) {
        // Verificar si hay IPs que han expirado
        check_expired_leases();
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
        printf("\033[1;31mNo hay más direcciones IP disponibles.\033[0m\n");
        send_dhcp_nak(sockfd, request, client_addr);
        return;
    }
    response.yiaddr = inet_addr(offered_ip); // Dirección IP ofrecida
    printf("Ofreciendo IP: %s\n", offered_ip);

    // Asignar el tiempo de lease
    for (int i = 0; i < MAX_IPS; i++) {
        if (strcmp(assigned_ips[i].ip, offered_ip) == 0) {
            assigned_ips[i].lease_remaining = ip_range->lease_time;
            assigned_ips[i].lease_start = time(NULL);  // Asignar el tiempo de inicio del lease
            printf("Lease asignado por %d segundos.\n", ip_range->lease_time);
            break;
        }
    }

    // Opciones DHCP (incluir información adicional como máscara de subred, gateway, etc.)
    response.options[0] = 53;  // DHCP Message Type
    response.options[1] = 1;   // Longitud de la opción
    response.options[2] = 2;   // DHCP Offer

    // Máscara de subred (opción 1)
    response.options[3] = 1;  // Opción: Máscara de subred
    response.options[4] = 4;  // Longitud de la opción
    response.options[5] = 255;
    response.options[6] = 255;
    response.options[7] = 255;
    response.options[8] = 0;  // Máscara: 255.255.255.0

    // Gateway (opción 3)
    response.options[9] = 3;  // Opción: Gateway
    response.options[10] = 4;  // Longitud de la opción
    response.options[11] = 192;
    response.options[12] = 168;
    response.options[13] = 1;
    response.options[14] = 1;  // Gateway: 192.168.1.1

    // Servidor DNS (opción 6)
    response.options[15] = 6;  // Opción: DNS
    response.options[16] = 4;  // Longitud de la opción
    response.options[17] = 8;
    response.options[18] = 8;
    response.options[19] = 8;
    response.options[20] = 8;  // DNS: 8.8.8.8 (Google DNS)

    // Fin de opciones (opción 255)
    response.options[21] = 255;  // Fin de opciones

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

    // Inicializar el estado de las IPs asignadas
    int start = 100;  // Empezamos en 192.168.1.100

    for (int i = 0; i < MAX_IPS; i++) {
        sprintf(assigned_ips[i].ip, "192.168.1.%d", start + i);  // Asignar el IP como cadena
        assigned_ips[i].available = 1;  // Marcar todas las IPs como disponibles al inicio
    }
}

// Función para asignar una dirección IP dinámica del rango
char* assign_ip_address(ip_range_t *ip_range) {
    static int next_ip_index = 0;  // Mantener el índice de la próxima IP
    int original_index = next_ip_index;  // Guardar el índice original para saber si dimos la vuelta completa

    // Buscar una IP disponible en el rango
    do {
        if (assigned_ips[next_ip_index].available == 1) {  // Si la IP está disponible
            assigned_ips[next_ip_index].available = 0;  // Marcarla como no disponible
            char* assigned_ip = assigned_ips[next_ip_index].ip;  // Obtener la IP
            next_ip_index = (next_ip_index + 1) % MAX_IPS;  // Avanzar el índice, con vuelta circular
            return assigned_ip;  // Devolver la IP asignada
        }
        next_ip_index = (next_ip_index + 1) % MAX_IPS;  // Avanzar al siguiente índice, con vuelta circular
    } while (next_ip_index != original_index);  // Continuar hasta dar una vuelta completa

    // Si llegamos aquí, no hay IPs disponibles
    return NULL;  // No se encontró una IP disponible
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

    // Buscar la IP en el arreglo de IPs y marcarla como no disponible
    for (int i = 0; i < MAX_IPS; i++) {
        if (strcmp(assigned_ips[i].ip, inet_ntoa(*(struct in_addr *)&request->yiaddr)) == 0) {
            assigned_ips[i].available = 0;  // Marcar como no disponible
            printf("IP %s marcada como no disponible.\n", assigned_ips[i].ip);
            break;
        }
    }

    // Opciones DHCP (incluimos la configuración adicional como máscara de subred, gateway, etc.)
    response.options[0] = 53;  // DHCP Message Type
    response.options[1] = 1;   // Longitud de la opción
    response.options[2] = 5;   // DHCP ACK

    // Máscara de subred (opción 1)
    response.options[3] = 1;  // Opción: Máscara de subred
    response.options[4] = 4;  // Longitud de la opción
    response.options[5] = 255;
    response.options[6] = 255;
    response.options[7] = 255;
    response.options[8] = 0;  // Máscara: 255.255.255.0

    // Gateway (opción 3)
    response.options[9] = 3;  // Opción: Gateway
    response.options[10] = 4;  // Longitud de la opción
    response.options[11] = 192;
    response.options[12] = 168;
    response.options[13] = 1;
    response.options[14] = 1;  // Gateway: 192.168.1.1

    // Servidor DNS (opción 6)
    response.options[15] = 6;  // Opción: DNS
    response.options[16] = 4;  // Longitud de la opción
    response.options[17] = 8;
    response.options[18] = 8;
    response.options[19] = 8;
    response.options[20] = 8;  // DNS: 8.8.8.8 (Google DNS)

    // Fin de opciones (opción 255)
    response.options[21] = 255;  // Fin de opciones

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
    // Verificar si la IP solicitada es válida y asignada
    for (int i = 0; i < MAX_IPS; i++) {
        if (assigned_ips[i].available == 0 && strcmp(assigned_ips[i].ip, inet_ntoa(*(struct in_addr *)&request->yiaddr)) == 0) {
            // Si la IP está asignada, renovamos el lease
            assigned_ips[i].lease_remaining = ip_range->lease_time;
            assigned_ips[i].lease_start = time(NULL);  // Reiniciar el tiempo de lease
            printf("Lease renovado para la IP %s por %d segundos.\n", assigned_ips[i].ip, ip_range->lease_time);
            
            // Enviar DHCPACK
            send_dhcp_ack(sockfd, request, client_addr, ip_range);
            return;
        }
    }

    // Si la IP no es válida o no está asignada, enviar DHCPNAK
    send_dhcp_nak(sockfd, request, client_addr);
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
    // Lógica para marcar la IP como disponible en el arreglo assigned_ips
    for (int i = 0; i < MAX_IPS; i++) {
        if (inet_addr(assigned_ips[i].ip) == ip) {  // Comparar la IP
            assigned_ips[i].available = 1;  // Marcarla como disponible
            printf("IP %s marcada como disponible.\n", assigned_ips[i].ip);
            break;
        }
    }
}

void check_expired_leases() {
    time_t current_time = time(NULL);  // Obtener el tiempo actual

    for (int i = 0; i < MAX_IPS; i++) {
        if (assigned_ips[i].available == 0) {  // Si la IP está asignada
            int time_elapsed = difftime(current_time, assigned_ips[i].lease_start);
            if (time_elapsed >= assigned_ips[i].lease_remaining) {
                // Si el tiempo de lease ha expirado
                printf("Lease para la IP %s ha expirado.\n", assigned_ips[i].ip);
                assigned_ips[i].available = 1;  // Marcar la IP como disponible
            }
        }
    }
}