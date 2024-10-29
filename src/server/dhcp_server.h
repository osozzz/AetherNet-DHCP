#ifndef DHCP_SERVER_H
#define DHCP_SERVER_H

// Bibliotecas necesarias
#include <arpa/inet.h>  // Para inet_ntop
#include <sys/socket.h> // Para sockets
#include <netinet/in.h> // Para sockaddr_in
#include <unistd.h>    // Para close
#include <pthread.h>    // Para hilos
#include <time.h>    // Para time
#include <string.h>    // Para memset
#include <errno.h>  // Para errno
#include <stdio.h>  // Para printf
#include <stdlib.h> // Para malloc, free
#include <signal.h> // Para signal
#include <stdint.h> // Para uint8_t, uint16_t, uint32_t
#include <sys/time.h> // Para timeval
#include <sys/ipc.h>    // Para ftok
#include <sys/msg.h>    // Para msgget, msgsnd, msgrcv

// Definiciones del servidor
#define DHCP_SERVER_PORT 67
#define BUFFER_SIZE 1024
#define MAX_IPS 10
#define HASH_TABLE_SIZE 256

// Estructuras de datos
typedef enum {
    DHCP_DISCOVER = 1,
    DHCP_OFFER,
    DHCP_REQUEST,
    DHCP_DECLINE,
    DHCP_ACK,
    DHCP_NAK,
    DHCP_RELEASE
} dhcp_message_type_t;

// Estructura para manejar la información de un cliente
typedef struct {
    int client_socket;
    struct sockaddr_in client_addr;
    const char* color;
    int client_id;  // Identificador del cliente
    int message_queue_id;
} client_info_t;

// Estructura para almacenar los hilos activos
typedef struct client_thread_info {
    uint32_t xid;            // ID de transacción del cliente
    uint8_t chaddr[6];       // Dirección MAC del cliente
    pthread_t thread_id;     // ID del hilo que maneja al cliente
    client_info_t* client_info; // Información del cliente asociada al hilo
    struct client_thread_info* next;  // Lista enlazada para manejar colisiones
} client_thread_info_t;

extern client_thread_info_t* client_threads[HASH_TABLE_SIZE];

// Definimos el rango de IPs con un identificador de pool
typedef struct {
    uint32_t start_ip;  // Dirección IP de inicio (entero de 32 bits)
    uint32_t end_ip;    // Dirección IP de fin (entero de 32 bits)
    int pool_id;        // Identificador del pool de IPs
} ip_range_t;

// Estructura para registrar las asignaciones de IP (nodo del árbol)
typedef struct ip_assignment_node {
    uint32_t ip;        // Dirección IP asignada (entero de 32 bits)
    uint8_t mac[6];     // Dirección MAC del cliente
    time_t lease_start; // Momento en que comenzó la concesión
    int lease_time;     // Tiempo de concesión para esta IP (en segundos)
    struct ip_assignment_node* left;  // Nodo izquierdo (IP menor)
    struct ip_assignment_node* right; // Nodo derecho (IP mayor)
} ip_assignment_node_t;

// Estructura que representa un paquete DHCP
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

// Estructura para el mensaje de la cola de mensajes
typedef struct dhcp_message {
    long mtype;               // Tipo de mensaje (obligatorio para las colas de mensajes)
    uint8_t buffer[BUFFER_SIZE];  // El contenido del mensaje (paquete DHCP)
    size_t length;            // Longitud del mensaje
} dhcp_message_t;

// Variables globales
extern int server_socket;          // Socket del servidor
extern int default_lease_time;     // Tiempo de concesión predeterminado (en segundos)
extern ip_assignment_node_t* ip_assignment_root;  // Raíz del árbol de asignaciones de IPs
extern int client_id_counter;      // Contador global para generar IDs únicos de cliente
extern ip_range_t global_ip_range; // Rango de IPs global
extern uint32_t subnet_mask;
extern uint32_t gateway_ip;
extern uint32_t dns_server_ip;
extern uint32_t server_ip;
extern const char* dhcp_server_ip;

// Mutexes para proteger el acceso a las variables globales
extern pthread_mutex_t ip_assignment_mutex;  // Mutex para proteger el acceso a la tabla de asignaciones de IPs
extern pthread_mutex_t client_id_mutex;  // Mutex para proteger el acceso al contador de IDs de cliente

// Prototipos de funciones

// Función para inicializar el servidor DHCP y configurar el socket, el rango de IPs, y los hilos
void init_dhcp_server(ip_range_t* range);

// Función para configurar el rango de IPs
void initialize_ip_pool(ip_range_t* range, const char* start_ip, const char* end_ip, int pool_id);

// Función principal para manejar el protocolo DHCP
void handle_dhcp_protocol(int sockfd);

// Función para crear un hilo que maneje a un cliente
void* client_handler(void* client_info);

//================================================

// Función para validar un paquete DHCP
int validate_dhcp_packet(struct dhcp_packet* packet);

// Función para manejar solicitudes DHCP DISCOVER
void handle_dhcp_discover(int sockfd, struct sockaddr_in* client_addr, struct dhcp_packet* request);

// Función para manejar solicitudes DHCP REQUEST
void handle_dhcp_request(int sockfd, struct sockaddr_in* client_addr, struct dhcp_packet* request);

// Función para manejar solicitudes DHCP DECLINE (cuando el cliente rechaza una IP)
void handle_dhcp_decline(int sockfd, struct sockaddr_in* client_addr, struct dhcp_packet* request);

// Función para manejar solicitudes DHCP RELEASE (cuando el cliente libera una IP)
void handle_dhcp_release(int sockfd, struct sockaddr_in* client_addr, struct dhcp_packet* request);

// Función para manejar las señales del servidor
void handle_signal(int signal);

//================================================

// Función para enviar opciones DHCP en el paquete
void send_dhcp_options(struct dhcp_packet* packet, int message_type, uint32_t assigned_ip);

// Función para buscar una opción DHCP en el paquete
uint8_t* find_dhcp_option(uint8_t* options, uint8_t code);

// Función para enviar un paquete DHCP OFFER en respuesta a DISCOVER
void send_dhcp_offer(int sockfd, struct sockaddr_in* client_addr, struct dhcp_packet* request, uint32_t assigned_ip);

// Función para enviar un paquete DHCP ACK en respuesta a un REQUEST
void send_dhcp_ack(int sockfd, struct sockaddr_in* client_addr, struct dhcp_packet* request, uint32_t assigned_ip);

// Función para enviar un paquete DHCP NAK (cuando el servidor no puede asignar una IP)
void send_dhcp_nak(int sockfd, struct sockaddr_in* client_addr, struct dhcp_packet* request);

//================================================

// Función para verificar si el lease de una IP ha expirado (para liberar IPs ocupadas)
ip_assignment_node_t* check_expired_leases(ip_assignment_node_t* root);

// Función para asignar una dirección IP a un cliente
uint32_t assign_ip_address(ip_range_t* range, struct dhcp_packet* request);

// Función para buscar una dirección IP en el árbol de asignaciones
ip_assignment_node_t* find_ip_assignment(ip_assignment_node_t* root, uint32_t ip);

// Función para insertar una nueva asignación en el árbol de IPs
ip_assignment_node_t* insert_ip_assignment(ip_assignment_node_t* root, uint32_t ip, uint8_t* mac, int lease_time);

// Función para eliminar una asignación del árbol de IPs (cuando se libera una IP)
ip_assignment_node_t* delete_ip_assignment(ip_assignment_node_t* root, uint32_t ip);

// Función para liberar la memoria de un árbol de asignaciones
void free_ip_assignment_tree(ip_assignment_node_t* root);  // Libera todo el árbol de asignaciones

// Función para convertir una cadena IP a entero
uint32_t ip_to_int(const char* ip_str); // Convierte una IP en cadena a entero

// Función para convertir un entero a cadena IP
char* int_to_ip(uint32_t ip_int); // Convierte un entero de 32 bits a cadena IP

// Función para obtener el tiempo restante de un lease
int get_lease_remaining(ip_assignment_node_t* assignment);

// Función para imprimir el árbol de asignaciones
void print_tree(ip_assignment_node_t* root);

// Función para encontrar el nodo con la IP mínima en un árbol
ip_assignment_node_t* find_min(ip_assignment_node_t* node);

// Función para manejar las señales del servidor
void cleanup();

//================================================================

// Función para buscar un cliente en la lista de clientes
client_thread_info_t* find_client_thread(uint8_t* chaddr);

// Función para agregar un cliente a la lista de clientes
void add_client_thread(uint32_t xid, uint8_t* chaddr, pthread_t thread_id, client_info_t* client_info);

// Función para eliminar un cliente de la lista de clientes
void remove_client_thread(uint8_t* chaddr);

unsigned int hash_mac(const uint8_t* mac);

#endif // DHCP_SERVER_H
