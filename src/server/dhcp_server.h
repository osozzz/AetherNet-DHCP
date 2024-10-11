#ifndef DHCP_SERVER_H
#define DHCP_SERVER_H

#include <arpa/inet.h>

// Puerto estándar del servidor DHCP
#define DHCP_SERVER_PORT 67

// Tamaño del buffer para recibir mensajes
#define BUFFER_SIZE 1024

// Prototipos de funciones que estarán en dhcp_server.c
void init_dhcp_server();
void handle_dhcp_requests(int sockfd);

#endif // DHCP_SERVER_H
