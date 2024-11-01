#ifndef DHCP_RELAY_H
#define DHCP_RELAY_H

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <sys/select.h>

// Tamaño de la tabla hash
#define HASH_TABLE_SIZE 256

// Estructura para almacenar información de transacción (cliente)
typedef struct dhcp_transaction {
    uint32_t xid;  // ID de transacción
    struct sockaddr_in client_addr;  // Dirección del cliente
    uint8_t mac_addr[6];  // Dirección MAC del cliente
    struct dhcp_transaction* next;  // Encadenamiento para colisiones en la tabla hash
} dhcp_transaction_t;

// Tabla hash de transacciones para almacenar las solicitudes de clientes
extern dhcp_transaction_t* hash_table[HASH_TABLE_SIZE];

// Funciones para la tabla hash
unsigned int hash_xid(uint32_t xid);  // Función hash basada en el xid
void insert_transaction(uint32_t xid, struct sockaddr_in* client_addr, uint8_t* mac_addr);  // Insertar transacción
dhcp_transaction_t* find_transaction(uint32_t xid);  // Buscar transacción por xid
void remove_transaction(uint32_t xid);  // Eliminar transacción

// Funciones principales del relay DHCP
void init_dhcp_relay();  // Inicializar el relay DHCP
void dhcp_relay_loop(int relay_sock, struct sockaddr_in* server_addr);  // Bucle principal del relay

#endif // DHCP_RELAY_H
