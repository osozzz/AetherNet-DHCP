#ifndef DHCP_CLIENT_H
#define DHCP_CLIENT_H

#include <arpa/inet.h>
#include <stdint.h>  // Para los tipos de datos uint8_t, uint32_t, etc.

#define DHCP_DISCOVER 1
#define DHCP_OFFER 2
#define DHCP_REQUEST 3
#define DHCP_DECLINE 4
#define DHCP_ACK 5
#define DHCP_NAK 6
#define DHCP_RELEASE 7
#define BUFFER_SIZE 1024
#define DHCP_CLIENT_PORT 68
#define DHCP_SERVER_PORT 67

// Estructura simplificada del paquete DHCP
struct dhcp_packet {
    uint8_t op;           // Tipo de mensaje (1 = solicitud)
    uint8_t htype;        // Tipo de hardware (1 = Ethernet)
    uint8_t hlen;         // Longitud de la dirección MAC
    uint8_t hops;         // Número de saltos (0)
    uint32_t xid;         // ID de transacción (único por cliente)
    uint16_t secs;        // Segundos transcurridos (0 en DISCOVER)
    uint16_t flags;       // Banderas
    uint32_t ciaddr;      // Dirección IP del cliente (0 en DISCOVER)
    uint32_t yiaddr;      // Dirección IP ofrecida por el servidor
    uint32_t siaddr;      // Dirección IP del servidor DHCP
    uint32_t giaddr;      // Dirección del gateway o relay
    uint8_t chaddr[16];   // Dirección MAC del cliente
    uint8_t sname[64];    // Nombre del servidor DHCP (opcional)
    uint8_t file[128];    // Archivo de arranque (opcional)
    uint8_t options[312]; // Opciones DHCP (53 para tipo de mensaje)
};

// Funciones del cliente DHCP
void send_dhcp_discover(int sockfd, struct sockaddr_in *server_addr);
void receive_dhcp_offer(int sockfd, struct in_addr *offered_ip);
int receive_dhcp_ack_or_nak(int sockfd);  // Nueva función para manejar DHCPACK o DHCPNAK
void send_dhcp_request(int sockfd, struct sockaddr_in *server_addr, struct in_addr *offered_ip);
void send_dhcp_decline(int sockfd, struct in_addr *offered_ip, struct sockaddr_in *server_addr);  // Manejo de DHCPDECLINE
void send_dhcp_release(int sockfd, struct sockaddr_in *server_addr, struct in_addr *assigned_ip);  // Manejo de DHCPRELEASE
void renew_dhcp_lease(int sockfd, struct sockaddr_in *server_addr);  // Renovar la concesión DHCP
void receive_dhcp_ack(int sockfd);  // Recibir DHCPACK

// Función que inicia el cliente DHCP
void start_dhcp_client();

// Función para verificar conflictos de IP
int check_ip_conflict(const char *ip_address);  // Nueva función para manejar el conflicto de IP (ping)
#endif
