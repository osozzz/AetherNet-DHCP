#include "dhcp_relay.h"

const int relay_port = 68;
const int server_port = 67;
dhcp_transaction_t* hash_table[HASH_TABLE_SIZE] = { NULL };

void init_dhcp_relay() {
    struct sockaddr_in relay_addr, server_addr;
    // Leer la IP del servidor DHCP desde una variable de entorno
    const char* server_ip = getenv("SERVER_IP");
    if (server_ip == NULL) {
        fprintf(stderr, "Error: La variable de entorno SERVER_IP no está definida.\n");
        exit(EXIT_FAILURE);
    }

    // Crear el socket para el relay
    int relay_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (relay_sock < 0) {
        perror("Error al crear el socket del relay");
        exit(EXIT_FAILURE);
    }
    printf("Socket del relay creado exitosamente.\n");

    // Configurar el socket del relay para escuchar en broadcast
    int broadcast_enable = 1;
    if (setsockopt(relay_sock, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(broadcast_enable)) < 0) {
        perror("Error al habilitar broadcast en el socket del relay");
        close(relay_sock);
        exit(EXIT_FAILURE);
    }

    // Configurar la dirección del relay
    memset(&relay_addr, 0, sizeof(relay_addr));
    relay_addr.sin_family = AF_INET;
    relay_addr.sin_port = htons(relay_port);  // Puerto del relay
    relay_addr.sin_addr.s_addr = htonl(INADDR_ANY);  // Escuchar en todas las interfaces

    // Bind del socket relay
    if (bind(relay_sock, (struct sockaddr*)&relay_addr, sizeof(relay_addr)) < 0) {
        perror("Error al hacer bind en el socket del relay");
        close(relay_sock);
        exit(EXIT_FAILURE);
    }

    // Configurar el servidor DHCP al cual se reenviarán los mensajes
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);  // Puerto del servidor DHCP
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("Error con la dirección IP del servidor DHCP");
        close(relay_sock);
        exit(EXIT_FAILURE);
    }

    printf("Relay DHCP iniciado en el puerto %d\n", ntohs(relay_addr.sin_port));

    // Aquí puedes manejar el bucle para reenviar los mensajes entre los clientes y el servidor
    dhcp_relay_loop(relay_sock, &server_addr);
}

void dhcp_relay_loop(int relay_sock, struct sockaddr_in* server_addr) {
    char buffer[1024];
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    fd_set read_fds;
    struct timeval timeout;

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(relay_sock, &read_fds);
        timeout.tv_sec = 120;
        timeout.tv_usec = 0;

        int activity = select(relay_sock + 1, &read_fds, NULL, NULL, &timeout);
        
        if (activity < 0) {
            perror("Error en select");
            continue;
        } else if (activity == 0) {
            printf("Timeout: No se recibió ningún mensaje del cliente en el tiempo establecido.\n");
            continue;
        }

        if (FD_ISSET(relay_sock, &read_fds)) {
            // Recibir mensaje del cliente o servidor
            ssize_t recv_len = recvfrom(relay_sock, buffer, sizeof(buffer), 0, (struct sockaddr*)&client_addr, &client_len);
            if (recv_len < 0) {
                perror("Error al recibir paquete");
                continue;
            }

            // Determinar si es una solicitud del cliente o una respuesta del servidor
            uint32_t xid;
            memcpy(&xid, buffer + 4, sizeof(xid));  // XID está en el offset 4 del paquete DHCP
        
            // Buscar la transacción en la tabla hash
            dhcp_transaction_t* transaction = find_transaction(xid);

            if(transaction == NULL) {
                uint8_t* mac_addr = (uint8_t*)(buffer + 28); // Dirección MAC del cliente en el offset 28
                insert_transaction(xid, &client_addr, mac_addr);
                printf("Solicitud DHCP recibida del cliente.\n");

                // Reenviar la solicitud al servidor DHCP
                ssize_t server_sent_len = sendto(relay_sock, buffer, recv_len, 0, (struct sockaddr*)server_addr, sizeof(*server_addr));
                if (server_sent_len < 0) {
                    perror("Error al reenviar la solicitud al servidor");
                    continue;
                }
                printf("Solicitud DHCP reenviada al servidor.\n");
            } else {
                printf("Paquete DHCP recibido desde el SERVIDOR: %s\n", inet_ntoa(server_addr->sin_addr));
                
                // Reenviar la respuesta al cliente
                ssize_t client_sent_len = sendto(relay_sock, buffer, recv_len, 0, (struct sockaddr*)&transaction->client_addr, client_len);
                if (client_sent_len < 0) {
                    perror("Error al reenviar la respuesta al cliente");
                    continue;
                }
                printf("Respuesta reenviada al cliente: %s\n", inet_ntoa(transaction->client_addr.sin_addr));

                // Eliminar la transacción una vez que se haya reenviado la respuesta
                remove_transaction(xid);
            }
        }
    }
}

// Función hash simple para el xid
unsigned int hash_xid(uint32_t xid) {
    return xid % HASH_TABLE_SIZE;
}

// Insertar transacción en la tabla hash
void insert_transaction(uint32_t xid, struct sockaddr_in* client_addr, uint8_t* mac_addr) {
    unsigned int hash_index = hash_xid(xid);

    dhcp_transaction_t* new_transaction = (dhcp_transaction_t*)malloc(sizeof(dhcp_transaction_t));
    new_transaction->xid = xid;
    new_transaction->client_addr = *client_addr;
    memcpy(new_transaction->mac_addr, mac_addr, 6);
    new_transaction->next = hash_table[hash_index];
    hash_table[hash_index] = new_transaction;
}

// Buscar transacción en la tabla hash por xid
dhcp_transaction_t* find_transaction(uint32_t xid) {
    unsigned int hash_index = hash_xid(xid);
    dhcp_transaction_t* transaction = hash_table[hash_index];

    while (transaction != NULL) {
        if (transaction->xid == xid) {
            return transaction;
        }
        transaction = transaction->next;
    }
    return NULL;
}

// Eliminar una transacción de la tabla hash después de reenviar
void remove_transaction(uint32_t xid) {
    unsigned int hash_index = hash_xid(xid);
    dhcp_transaction_t* current = hash_table[hash_index];
    dhcp_transaction_t* previous = NULL;

    while (current != NULL && current->xid != xid) {
        previous = current;
        current = current->next;
    }

    if (current == NULL) {
        // Transacción no encontrada
        return;
    }

    if (previous == NULL) {
        // El primer elemento en la lista
        hash_table[hash_index] = current->next;
    } else {
        previous->next = current->next;
    }

    free(current);  // Liberar la memoria
}
