#ifndef DHCP_CLIENT_H
#define DHCP_CLIENT_H

#include <arpa/inet.h>

#define DHCP_DISCOVER 1
#define DHCP_OFFER 2
#define BUFFER_SIZE 1024
#define DHCP_CLIENT_PORT 68
#define DHCP_SERVER_PORT 67

// Estructura simplificada del paquete DHCP
struct dhcp_packet {
    uint8_t op;           // Tipo de mensaje
    uint8_t htype;        // Tipo de hardware
    uint8_t hlen;         // Longitud de la dirección MAC
    uint8_t hops;         // Número de saltos
    uint32_t xid;         // ID de transacción
    uint16_t secs;        // Segundos transcurridos
    uint16_t flags;       // Banderas
    uint32_t ciaddr;      // Dirección IP del cliente (si tiene una)
    uint32_t yiaddr;      // Dirección IP ofrecida por el servidor
    uint32_t siaddr;      // Dirección IP del servidor DHCP
    uint32_t giaddr;      // Dirección del gateway o relay
    uint8_t chaddr[16];   // Dirección MAC del cliente
    uint8_t options[312]; // Opciones DHCP
};

// Funciones del cliente DHCP
void send_dhcp_discover(int sockfd, struct sockaddr_in *server_addr);
void receive_dhcp_offer(int sockfd);

#endif
