#ifndef DHCP_CLIENT_H
#define DHCP_CLIENT_H

#include <arpa/inet.h>  // Para inet_ntop
#include <sys/socket.h> // Para sockets
#include <netinet/in.h> // Para sockaddr_in
#include <unistd.h>     // Para close
#include <pthread.h>    // Para hilos
#include <time.h>       // Para time
#include <string.h>     // Para memset
#include <errno.h>      // Para errno
#include <stdio.h>      // Para printf
#include <stdlib.h>     // Para malloc, free
#include <signal.h>     // Para signal
#include <stdint.h>     // Para uint8_t, uint16_t, uint32_t
#include <sys/time.h>   // Para timeval
#include <time.h>

// Tipos de mensajes DHCP
typedef enum {
    DHCP_DISCOVER = 1,
    DHCP_OFFER,
    DHCP_REQUEST,
    DHCP_DECLINE,
    DHCP_ACK,
    DHCP_NAK,
    DHCP_RELEASE
} dhcp_message_type_t;

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
} __attribute__((packed));

// Estructura para manejar el cliente DHCP, incluyendo el control del lease
typedef struct dhcp_client {
    int sockfd;               // Socket del cliente
    uint32_t lease_time;       // Tiempo de concesión del lease
    uint32_t renewal_time;     // Tiempo para enviar la renovación (T1)
    uint32_t rebind_time;      // Tiempo para rebind (T2)
    int nak_attempts;          // Contador de reintentos de NAK
    int decline_attempts;      // Contador de reintentos de DECLINE
    struct in_addr offered_ip; // IP ofrecida por el servidor
    struct in_addr server_ip;  // IP del servidor DHCP
    struct sockaddr_in server_addr;  // Dirección completa del servidor (sockaddr_in)
    struct in_addr subnet_mask;  // Máscara de subred
    struct in_addr network_ip;   // IP de red base
    uint8_t mac_addr[6];
} dhcp_client_t;

// Funciones públicas (declaraciones)
void init_dhcp_client();  // Función que inicializa el cliente DHCP

void handle_dhcp_protocol(dhcp_client_t* client, struct sockaddr_in* server_addr);  // Manejo del ciclo DHCP

void handle_dhcp_offer(dhcp_client_t* client, struct sockaddr_in* server_addr, struct dhcp_packet* offer_packet);  // Procesar la oferta DHCP
void handle_dhcp_ack(dhcp_client_t* client, struct dhcp_packet* ack_packet, struct sockaddr_in* server_addr);      // Procesar el ACK
void handle_dhcp_nak(dhcp_client_t* client);                                     // Procesar el NAK

void send_dhcp_discover(int sockfd, struct sockaddr_in* server_add, uint8_t* mac_addr);            // Enviar DHCPDISCOVER
void send_dhcp_request(int sockfd, struct sockaddr_in* server_addr, struct in_addr* offered_ip, uint8_t* mac_addr); // Enviar DHCPREQUEST
void send_dhcp_decline(int sockfd, struct in_addr* offered_ip, struct sockaddr_in* server_addr, uint8_t* mac_addr);  // Enviar DHCPDECLINE
void send_dhcp_release(dhcp_client_t* client, struct sockaddr_in* server_addr, struct in_addr* offered_ip);  // Enviar DHCPRELEASE

int validate_offered_ip(struct in_addr* offered_ip, struct in_addr* subnet_mask, struct in_addr* network_ip); // Validar la IP ofrecida
void handle_lease_renewal(dhcp_client_t* client, struct sockaddr_in* server_addr, struct in_addr* offered_ip);  // Manejo de la renovación del lease

// Funciones auxiliares para agregar y buscar opciones DHCP
uint8_t* find_dhcp_option(uint8_t* options, uint8_t code);
void generate_random_mac(uint8_t* mac_addr);

#endif  // DHCP_CLIENT_H
