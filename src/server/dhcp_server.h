#ifndef DHCP_SERVER_H
#define DHCP_SERVER_H

#include <arpa/inet.h>

#define DHCP_SERVER_PORT 67
#define BUFFER_SIZE 1024
#define MAX_IPS 10  // Número máximo de direcciones IP que podemos asignar

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
    time_t lease_start; // Tiempo de concesión
    int lease_remaining; // Tiempo de concesión restante
    uint8_t available;  // 1 = disponible, 0 = no disponible
} ip_assignment_t;

typedef struct {
    int sockfd;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len;
    char *buffer;
} client_request_args_t;


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
    unsigned char chaddr[16];
    unsigned char sname[64];
    unsigned char file[128];
    unsigned char options[312];
};

// Prototipos de funciones
void init_dhcp_server();

// Función para manejar las solicitudes DHCP
void handle_dhcp_requests(int sockfd);

// Funciones para enviar respuestas DHCP
void send_dhcp_offer(int sockfd, struct dhcp_packet *request, struct sockaddr_in *client_addr, ip_range_t *ip_range);
void handle_dhcp_request(int sockfd, struct dhcp_packet *request, struct sockaddr_in *client_addr, ip_range_t *ip_range);
void send_dhcp_ack(int sockfd, struct dhcp_packet *request, struct sockaddr_in *client_addr, ip_range_t *ip_range);
void send_dhcp_nak(int sockfd, struct dhcp_packet *request, struct sockaddr_in *client_addr);

// Funciones para manejar mensajes adicionales
void handle_dhcp_decline(int sockfd, struct dhcp_packet *decline_message, ip_range_t *ip_range);
void handle_dhcp_release(int sockfd, struct dhcp_packet *release_message, ip_range_t *ip_range);

// Función para inicializar el rango de IPs
void initialize_ip_range(ip_range_t *range, const char *start_ip, const char *end_ip, int lease_time);

// Funciones auxiliares para marcar IPs
void mark_ip_as_unavailable(ip_range_t *ip_range, uint32_t ip);
void mark_ip_as_available(ip_range_t *ip_range, uint32_t ip);

// Función para asignar una dirección IP
char* assign_ip_address(ip_range_t *ip_range);

// Función para liberar una dirección IP
void check_expired_leases();


// Funciones para manejar las solicitudes DHCP en un hilo
void* handle_dhcp_request_thread(void* arg);

void* dhcp_handler(void* arg);

void shutdown_dhcp_server();

void *handle_client_request(void *args);

#endif // DHCP_SERVER_H
