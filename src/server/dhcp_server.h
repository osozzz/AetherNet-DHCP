#ifndef DHCP_SERVER_H
#define DHCP_SERVER_H

#include <arpa/inet.h>

#define DHCP_SERVER_PORT 67
#define BUFFER_SIZE 1024
#define MAX_IPS 50  // Número máximo de direcciones IP que podemos asignar

// Estructura para manejar el rango de direcciones IP
typedef struct {
    char* start_ip; // Dirección IP de inicio
    char* end_ip;   // Dirección IP de fin
    int lease_time; // Tiempo de concesión en segundos
} ip_range_t;

// Estructura para registrar las asignaciones de IP
typedef struct {
    char ip[16];        // Dirección IP asignada
    uint8_t mac[6];     // Dirección MAC del cliente
    int lease_remaining; // Tiempo de concesión restante
} ip_assignment_t;

// Estructura básica del paquete DHCP (simplificado)
struct dhcp_packet {
    uint8_t op;           // Message type
    uint8_t htype;        // Hardware type
    uint8_t hlen;         // Hardware address length
    uint8_t hops;         // Number of hops
    uint32_t xid;         // Transaction ID
    uint16_t secs;        // Seconds elapsed
    uint16_t flags;       // Flags
    uint32_t ciaddr;      // Client IP address
    uint32_t yiaddr;      // 'Your' (client) IP address
    uint32_t siaddr;      // Server IP address
    uint32_t giaddr;      // Gateway IP address
    uint8_t chaddr[16];   // Client hardware address (MAC)
    uint8_t options[312]; // DHCP options (variable length)
};

// Prototipos de funciones
void init_dhcp_server();
void handle_dhcp_requests(int sockfd);
void send_dhcp_offer(int sockfd, struct dhcp_packet *request, struct sockaddr_in *client_addr, ip_range_t *ip_range);
char* assign_ip_address(ip_range_t *ip_range);
void initialize_ip_range(ip_range_t *ip_range, const char* start_ip, const char* end_ip, int lease_time);

#endif // DHCP_SERVER_H
