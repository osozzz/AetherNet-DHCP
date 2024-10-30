#include "dhcp_server.h"

// Definicion de Variables globales
int server_socket;                 // Socket del servidor
int default_lease_time = 30;       // Tiempo de concesión predeterminado en segundos
ip_assignment_node_t* ip_assignment_root = NULL; // Raíz del árbol de asignaciones de IPs
int client_id_counter = 1;         // Contador global para IDs de cliente
ip_range_t global_ip_range;        // Rango global de IPs
uint32_t subnet_mask;
uint32_t gateway_ip;
uint32_t dns_server_ip;
uint32_t server_ip;
const char* dhcp_server_ip = "172.19.2.228";  // IP del servidor DHCP
static uint32_t last_assigned_ip = 0;

const char* colors[] = {
    "\033[31m", // Rojo
    "\033[32m", // Verde
    "\033[33m", // Amarillo
    "\033[34m", // Azul
    "\033[35m", // Magenta
    "\033[36m", // Cian
};

const char* reset_color = "\033[0m";  // Restablecer el color de la consola

// Mutexes para proteger el acceso a las variables globales
pthread_mutex_t ip_assignment_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t client_id_mutex = PTHREAD_MUTEX_INITIALIZER;

// Definicion de la tabla de hash para los hilos de los clientes
client_thread_info_t* client_threads[HASH_TABLE_SIZE] = {NULL}; // Inicializar la tabla en NULL

// Funciones para el servidor DHCP
void init_dhcp_server(ip_range_t* range) {  // Inicializar el servidor DHCP
    struct sockaddr_in server_addr, relay_addr;

    /// Inicializar las variables IP
    if (inet_pton(AF_INET, "255.255.255.0", &subnet_mask) != 1) {
        perror("Error al convertir la máscara de subred");
        exit(EXIT_FAILURE);
    }
    
    if (inet_pton(AF_INET, dhcp_server_ip, &gateway_ip) != 1) {
        perror("Error al convertir la IP del gateway");
        exit(EXIT_FAILURE);
    }
    
    if (inet_pton(AF_INET, "8.8.8.8", &dns_server_ip) != 1) {
        perror("Error al convertir la IP del servidor DNS");
        exit(EXIT_FAILURE);
    }
    
    if (inet_pton(AF_INET, dhcp_server_ip, &server_ip) != 1) {
        perror("Error al convertir la IP del servidor DHCP");
        exit(EXIT_FAILURE);
    }
    
    global_ip_range = *range;
    pthread_mutex_init(&ip_assignment_mutex, NULL);
    pthread_mutex_init(&client_id_mutex, NULL);

    // Crear el socket UDP del servidor
    server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_socket < 0) {
        perror("Error al crear el socket del servidor");
        cleanup();
        exit(EXIT_FAILURE);
    }
    printf("Socket del servidor creado exitosamente.\n");

    // Configurar el socket para recibir paquetes de broadcast
    int broadcast_enable = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(broadcast_enable)) < 0) {
        perror("Error al configurar el socket del servidor para broadcast");
        cleanup();
        exit(EXIT_FAILURE);
    }

    // Configurar la dirección del servidor
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(DHCP_SERVER_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;  // Escuchar en cualquier interfaz
    
    // Bind del socket del servidor
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error al hacer bind en el socket del servidor");
        cleanup();
        exit(EXIT_FAILURE);
    }
    
    //  Configurar la dirección del relay
    memset(&relay_addr, 0, sizeof(relay_addr));
    relay_addr.sin_family = AF_INET;
    relay_addr.sin_addr.s_addr = INADDR_ANY;  // Escuchar en cualquier interfaz
    relay_addr.sin_port = 68;  // Puerto del relay (67 o 68)

    // Configurar timeout en el socket
    struct timeval timeout;
    timeout.tv_sec = 120;  // Tiempo de espera de 5 segundos
    timeout.tv_usec = 0;

    if (setsockopt(server_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("Error al establecer el tiempo de espera para el socket del servidor");
        cleanup();
        exit(EXIT_FAILURE);
    }

    printf("Servidor DHCP iniciado en el puerto %d\n", DHCP_SERVER_PORT);

    // A partir de aquí, el servidor podría empezar a escuchar las solicitudes de los clientes.
    handle_dhcp_protocol(server_socket);
}

void handle_dhcp_protocol(int sockfd) {
    struct sockaddr_in client_addr;
    uint8_t buffer[BUFFER_SIZE];  // Buffer para recibir los datos
    socklen_t addr_len = sizeof(client_addr);
    struct dhcp_packet* request;

    while (1) {
        check_expired_leases(ip_assignment_root);

        // Esperar y recibir una solicitud DHCP
        ssize_t message = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, 
                                          (struct sockaddr*)&client_addr, &addr_len);
        if (message < 0) {
            perror("Error al recibir datos del cliente");
            sleep(1);  // Evitar un ciclo rápido de errores
            continue;
        }

        // Crear una estructura DHCP para el paquete recibido
        request = (struct dhcp_packet *)buffer;

        // Copiar los datos del buffer a la estructura DHCP (ajustar según lo necesario)
        memcpy(request, buffer, message);

        // Validar el paquete DHCP recibido
        if (!validate_dhcp_packet(request)) {
            fprintf(stderr, "Error: Paquete DHCP inválido de %s.\n", inet_ntoa(client_addr.sin_addr));
            continue;
        }

        // Verificar si ya existe un hilo manejando este cliente (chaddr)
        client_thread_info_t* existing_thread = find_client_thread(request->chaddr);

        if (existing_thread != NULL) {
            printf("Reenviando solicitud DHCP al hilo existente para el cliente %02x:%02x:%02x:%02x:%02x:%02x\n",
                   request->chaddr[0], request->chaddr[1], request->chaddr[2],
                   request->chaddr[3], request->chaddr[4], request->chaddr[5]);

            // Configurar la estructura del mensaje DHCP antes de enviarlo
            dhcp_message_t msg;
            msg.mtype = 1;  // Puedes usar cualquier identificador válido para el tipo de mensaje
            memcpy(msg.buffer, request, message);  // Copiar los datos DHCP al buffer
            printf("Se copia bien los datos DHCP al buffer\n");
            msg.length = sizeof(msg.buffer);  // Establecer el tamaño real del mensaje recibido

            // Enviar el mensaje a la cola del hilo correspondiente
            if (msgsnd(existing_thread->client_info->message_queue_id, (void * ) &msg, msg.length, 0) == -1) {
                perror("Error al enviar mensaje a la cola del hilo del cliente");
                // Agregar mensaje de error para seguimiento
                fprintf(stderr, "Error al enviar el mensaje de cliente MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
                    request->chaddr[0], request->chaddr[1], request->chaddr[2],
                    request->chaddr[3], request->chaddr[4], request->chaddr[5]);
                continue;
            }
            printf("Enviado correctamente a la cola del hilo del cliente\n");
            continue;
        }

        // Validar si es un paquete DISCOVER (opción 53, tipo de mensaje DHCP)
        uint8_t* message_type = find_dhcp_option(request->options, 53);
        if (!message_type) {
            fprintf(stderr, "Error: No se encontró la opción de tipo de mensaje DHCP.\n");
            continue;
        }

        if (message_type && *message_type == DHCP_DISCOVER) {
            printf("Solicitud DHCP DISCOVER recibida de %s\n", inet_ntoa(client_addr.sin_addr));

            // Crear una estructura client_info_t para el nuevo hilo
            client_info_t* client_info = (client_info_t*)malloc(sizeof(client_info_t));
            if (!client_info) {
                perror("Error al asignar memoria para el cliente");
                continue;
            }
            client_info->client_socket = sockfd;
            client_info->client_addr = client_addr;
            pthread_mutex_lock(&client_id_mutex);
            client_info->client_id = client_id_counter++;
            pthread_mutex_unlock(&client_id_mutex);
            client_info->color = colors[client_info->client_id % 6];
            client_info->message_queue_id = msgget(IPC_PRIVATE, IPC_CREAT | 0666);  // Crear cola de mensajes privada
            if (client_info->message_queue_id == -1) {
                perror("Error al crear la cola de mensajes");
                free(client_info);
                continue;
            }

            memcpy(&client_info->initial_request, request, sizeof(struct dhcp_packet));  // Copiar el paquete inicial

            // Crear un hilo para manejar el cliente
            pthread_t thread_id;
            if (pthread_create(&thread_id, NULL, client_handler, (void*)client_info) != 0) {
                perror("Error al crear el hilo para el cliente");
                free(client_info);
                continue;
            }

            // Agregar el hilo a la lista de hilos de clientes
            add_client_thread(request->chaddr, thread_id, client_info);
            // Detach del hilo para que se libere automáticamente cuando termine
            pthread_detach(thread_id);
        } else {
            fprintf(stderr, "Error: Paquete DHCP no reconocido.\n");
        }
    }
}

void* client_handler(void* client_info) {
    client_info_t* info = (client_info_t*)client_info;
    struct sockaddr_in client_addr = info->client_addr;
    int sockfd = info->client_socket;
    struct dhcp_packet request;
    dhcp_message_t msg;
    long msgtyp = 0;

    // Imprimir el ID del hilo con el color asignado
    printf("%sID del hilo: %p asignado al cliente %d\n%s", info->color, (void*)pthread_self(), info->client_id, reset_color);

    // Manejo inicial: procesar el DHCP DISCOVER recibido
    printf("Procesando DHCP DISCOVER inicial del cliente.\n");
    handle_dhcp_discover(sockfd, &client_addr, &info->initial_request);  // Procesar DISCOVER

    int ack_sent = 0;  // Bandera para verificar si se ha enviado un ACK
    int done = 0;      // Bandera para finalizar el ciclo del cliente
    
    while (done < 1) {
        // Recibir un mensaje de la cola (bloquea hasta recibir una solicitud)
        if (msgrcv(info->message_queue_id, (void *) &msg, sizeof(msg.buffer), msgtyp, MSG_NOERROR | IPC_NOWAIT) == -1) {
            continue;
        }
        // Verificar que el tamaño del mensaje sea válido
        if (sizeof(msg.buffer) > BUFFER_SIZE) {
            fprintf(stderr, "Error: Tamaño del mensaje excede el tamaño del paquete DHCP (%u bytes).\n", BUFFER_SIZE);
            continue;
        }

        // Copiar el contenido del mensaje en el paquete DHCP
        memcpy(&request, msg.buffer, sizeof(msg.buffer));
        // Validar el tipo de solicitud DHCP (opción 53) y procesar según el tipo
        uint8_t* message_type = find_dhcp_option(request.options, 53);
        if (message_type) {
            switch (*message_type) {
                case DHCP_DISCOVER:
                    printf("Solicitud DHCP DISCOVER recibida nuevamente.\n");
                    handle_dhcp_discover(sockfd, &client_addr, &request);
                    break;

                case DHCP_REQUEST:
                    if (ack_sent) {
                        // El cliente está solicitando renovar su lease
                        printf("Solicitud DHCP REQUEST de renovación recibida. Renovando lease.\n");
                        uint32_t requested_ip = ntohl(request.ciaddr);  // IP solicitada
                        ip_assignment_node_t* assignment = find_ip_assignment(ip_assignment_root, requested_ip);
                        if (assignment != NULL && memcmp(assignment->mac, request.chaddr, 6) == 0) {
                            printf("Renovando el lease para la IP %s\n", int_to_ip(requested_ip));
                            assignment->lease_start = time(NULL);  // Reiniciar lease time
                        } else {
                            printf("Error: No se pudo renovar el lease para la IP %s\n", int_to_ip(requested_ip));
                        }
                    } else {
                        printf("Solicitud DHCP REQUEST recibida.\n");
                        handle_dhcp_request(sockfd, &client_addr, &request);
                        ack_sent = 1;  // Se ha enviado un ACK inicial
                    }
                    break;

                case DHCP_DECLINE:
                    printf("Solicitud DHCP DECLINE recibida. Cerrando conexión.\n");
                    handle_dhcp_decline(sockfd, &client_addr, &request);
                    done = 1;  // Terminar el ciclo
                    break;

                case DHCP_RELEASE:
                    printf("Solicitud DHCP RELEASE recibida. Cerrando conexión.\n");
                    handle_dhcp_release(sockfd, &client_addr, &request);
                    done = 1;  // Terminar el ciclo
                    break;

                default:
                    printf("Solicitud DHCP no reconocida.\n");
                    break;
            }
        } else {
            printf("Solicitud DHCP sin tipo válido.\n");
        }
    }

    // Eliminar la cola de mensajes del cliente y liberar la memoria
    msgctl(info->message_queue_id, IPC_RMID, NULL);
    printf("%sCliente %d desconectado. Cerrando hilo...\n%s", info->color, info->client_id, reset_color);
    free(client_info);  // Liberar la memoria del cliente
    pthread_exit(NULL);  // Terminar el hilo correctamente
    return NULL;  // Terminar el hilo correctamente
}

// Valida un paquete DHCP
int validate_dhcp_packet(struct dhcp_packet* packet) {
    if (!packet) {
        fprintf(stderr, "Error: Paquete DHCP nulo.\n");
        return 0;
    }
    
    // Validar el tipo de hardware y la longitud de la dirección de hardware (para Ethernet y MAC de 6 bytes)
    if (packet->hlen != 6 || packet->htype != 1) {
        fprintf(stderr, "Error: Paquete DHCP inválido (Tipo de hardware o longitud de dirección incorrectos).\n");
        return 0;
    }

    // Validar que el paquete tenga un tipo de mensaje DHCP válido (opción 53)
    uint8_t* message_type = find_dhcp_option(packet->options, 53);  // Buscar directamente la opción 53 en el array fijo de opciones
    if (!message_type) {
        fprintf(stderr, "Error: Paquete DHCP sin tipo de mensaje. XID: %u, MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
                packet->xid,
                packet->chaddr[0], packet->chaddr[1], packet->chaddr[2],
                packet->chaddr[3], packet->chaddr[4], packet->chaddr[5]);
        return 0;
    }

    return 1;  // Paquete válido
}

void handle_dhcp_discover(int sockfd, struct sockaddr_in* client_addr, struct dhcp_packet* request) {
    // Validar que el paquete sea un DISCOVER, por seguridad
    uint8_t* message_type = find_dhcp_option(request->options, 53);
    if (!message_type || *message_type != DHCP_DISCOVER) {
        printf("Error: El paquete no es un DISCOVER.\n");
        return;
    }
    // Asignar una dirección IP al cliente
    uint32_t assigned_ip = assign_ip_address(&global_ip_range, request);  // Usa el rango global de IPs
    if (assigned_ip == 0) {
        // No se pudo asignar una IP, imprime la dirección MAC del cliente
        printf("No se pudo asignar una dirección IP para el cliente con MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
               request->chaddr[0], request->chaddr[1], request->chaddr[2],
               request->chaddr[3], request->chaddr[4], request->chaddr[5]);
        send_dhcp_nack(sockfd, client_addr, request);
        return;
    }

    // Enviar la oferta DHCP OFFER al cliente
    send_dhcp_offer(sockfd, client_addr, request, assigned_ip);
}

void handle_dhcp_request(int sockfd, struct sockaddr_in* client_addr, struct dhcp_packet* request) {
    // Obtener la IP solicitada desde la opción 50 o ciaddr
    uint8_t* requested_ip_option = find_dhcp_option(request->options, 50);
    uint32_t requested_ip = 0;
    if (requested_ip_option) {
        requested_ip = ntohl(*(uint32_t*)requested_ip_option);  // Convertir a formato de host
    } else if (request->ciaddr != 0) {
        requested_ip = ntohl(request->ciaddr);  // Usar ciaddr si no se encuentra la opción 50 y ciaddr no es 0
    } else {
        printf("Error: No se especificó ninguna IP solicitada.\n");
        send_dhcp_nak(sockfd, client_addr, request);
        return;
    }

    // Verificar si la IP solicitada está dentro del rango
    if (requested_ip < global_ip_range.start_ip || requested_ip > global_ip_range.end_ip) {
        printf("La IP solicitada %s por el cliente con MAC %02x:%02x:%02x:%02x:%02x:%02x está fuera del rango.\n",
               int_to_ip(requested_ip),
               request->chaddr[0], request->chaddr[1], request->chaddr[2],
               request->chaddr[3], request->chaddr[4], request->chaddr[5]);
        send_dhcp_nak(sockfd, client_addr, request);  // Enviar NAK si la IP está fuera del rango
        return;
    }

    // Buscar la IP en el árbol de asignaciones para ver si está disponible
    ip_assignment_node_t* assignment = find_ip_assignment(ip_assignment_root, requested_ip);
    if (assignment != NULL && memcmp(assignment->mac, request->chaddr, 6) == 0) {
        // El cliente está solicitando su propia IP, enviar ACK
        printf("El cliente está solicitando su propia IP %s. Enviando ACK.\n", int_to_ip(requested_ip));
        send_dhcp_ack(sockfd, client_addr, request, requested_ip);
        return;
    }

    if (assignment == NULL) {
        // La IP no está asignada a nadie, enviamos DHCP ACK
        printf("La IP solicitada %s está disponible. Enviando ACK.\n", int_to_ip(requested_ip));
        send_dhcp_ack(sockfd, client_addr, request, requested_ip);
        ip_assignment_root = insert_ip_assignment(ip_assignment_root, requested_ip, request->chaddr, default_lease_time);
        if (!ip_assignment_root) {
            printf("Error al asignar la IP %s al cliente. Enviando NAK.\n", int_to_ip(requested_ip));
            send_dhcp_nak(sockfd, client_addr, request);
        }
    } else {
        // La IP ya está asignada a alguien más
        printf("La IP solicitada %s ya está asignada a otro cliente. Enviando NAK.\n", int_to_ip(requested_ip));
        send_dhcp_nak(sockfd, client_addr, request);  // Enviar NAK
    }
}

void handle_dhcp_decline(int sockfd, struct sockaddr_in* client_addr, struct dhcp_packet* request) {
    // Obtener la IP que el cliente está rechazando (yiaddr en el paquete DHCP)
    uint32_t declined_ip = ntohl(request->yiaddr);

    printf("Solicitud DHCP DECLINE recibida para la IP: %s del cliente con MAC %02x:%02x:%02x:%02x:%02x:%02x\n",
           int_to_ip(declined_ip),
           request->chaddr[0], request->chaddr[1], request->chaddr[2],
           request->chaddr[3], request->chaddr[4], request->chaddr[5]);

    // Verificar si la IP rechazada está asignada a alguien en el árbol de asignaciones
    ip_assignment_node_t* assignment = find_ip_assignment(ip_assignment_root, declined_ip);

    if (assignment != NULL) {
        // La IP está asignada, imprimir información sobre la asignación
        printf("La IP %s está asignada al cliente con MAC %02x:%02x:%02x:%02x:%02x:%02x. Será liberada.\n",
               int_to_ip(declined_ip),
               assignment->mac[0], assignment->mac[1], assignment->mac[2],
               assignment->mac[3], assignment->mac[4], assignment->mac[5]);

        // Eliminar la asignación de la IP del árbol
        ip_assignment_root = delete_ip_assignment(ip_assignment_root, declined_ip);
        printf("La IP %s ha sido liberada tras un DECLINE.\n", int_to_ip(declined_ip));
    } else {
        // Si no está asignada, solo lo registramos
        printf("La IP %s no estaba asignada, pero fue rechazada.\n", int_to_ip(declined_ip));
    }
}

void handle_dhcp_release(int sockfd, struct sockaddr_in* client_addr, struct dhcp_packet* request) {
    // Obtener la IP que el cliente está liberando (ciaddr en el paquete DHCP)
    uint32_t released_ip = ntohl(request->ciaddr);

    printf("Solicitud DHCP RELEASE recibida para la IP: %s del cliente con MAC %02x:%02x:%02x:%02x:%02x:%02x\n",
           int_to_ip(released_ip),
           request->chaddr[0], request->chaddr[1], request->chaddr[2],
           request->chaddr[3], request->chaddr[4], request->chaddr[5]);

    // Verificar si la IP liberada está asignada a alguien en el árbol de asignaciones
    ip_assignment_node_t* assignment = find_ip_assignment(ip_assignment_root, released_ip);

    if (assignment != NULL) {
        // Imprimir información sobre el cliente que tenía asignada la IP
        printf("La IP %s está asignada al cliente con MAC %02x:%02x:%02x:%02x:%02x:%02x. Será liberada.\n",
               int_to_ip(released_ip),
               assignment->mac[0], assignment->mac[1], assignment->mac[2],
               assignment->mac[3], assignment->mac[4], assignment->mac[5]);

        // Eliminar la asignación de la IP del árbol
        ip_assignment_root = delete_ip_assignment(ip_assignment_root, released_ip);
        printf("La IP %s ha sido liberada por el cliente.\n", int_to_ip(released_ip));
    } else {
        // Si no está asignada, solo lo registramos
        printf("La IP %s no estaba asignada, pero fue liberada por el cliente.\n", int_to_ip(released_ip));
    }
}

void send_dhcp_offer(int sockfd, struct sockaddr_in* client_addr, struct dhcp_packet* request, uint32_t assigned_ip) {
    struct dhcp_packet offer;

    // Limpiar la estructura del paquete DHCP OFFER
    memset(&offer, 0, sizeof(offer));

    // Configurar los campos principales del paquete DHCP
    offer.op = 2;  // 2 = BOOTREPLY (respuesta del servidor)
    offer.htype = 1;  // 1 = Ethernet
    offer.hlen = 6;   // Longitud de la dirección de hardware (6 para MAC)
    offer.xid = request->xid;  // Reutilizamos el ID de transacción del cliente
    offer.flags = request->flags;  // Reutilizamos los flags del cliente
    memcpy(offer.chaddr, request->chaddr, 6);  // Copiamos solo la dirección MAC (6 bytes)

    // Configurar las direcciones IP
    offer.yiaddr = htonl(assigned_ip);  // Dirección IP asignada al cliente
    offer.siaddr = htonl(server_ip);  // IP del servidor DHCP (esto lo debes definir previamente)

    // Agregar las opciones DHCP
    send_dhcp_options(&offer, DHCP_OFFER, assigned_ip);

    // Calcular el tamaño del paquete DHCP: base del paquete más las opciones
    ssize_t packet_size = sizeof(struct dhcp_packet) - sizeof(offer.options) + 312;  // Ajustar si las opciones varían

    // Enviar el paquete OFFER al cliente
    ssize_t sent_bytes = sendto(sockfd, &offer, packet_size, 0, 
                                (struct sockaddr*)client_addr, sizeof(*client_addr));
    if (sent_bytes < 0) {
        perror("Error al enviar DHCP OFFER");
    } else {
        printf("DHCP OFFER enviado a %s con la IP: %s\n", inet_ntoa(client_addr->sin_addr), int_to_ip(assigned_ip));
    }
}

void send_dhcp_ack(int sockfd, struct sockaddr_in* client_addr, struct dhcp_packet* request, uint32_t requested_ip) {
    struct dhcp_packet ack;

    // Limpiar la estructura
    memset(&ack, 0, sizeof(struct dhcp_packet));

    // Configurar los campos principales del paquete DHCP
    ack.op = 2;  // 2 = BOOTREPLY (respuesta del servidor)
    ack.htype = 1;  // 1 = Ethernet
    ack.hlen = 6;   // Longitud de la dirección de hardware (6 para MAC)
    ack.xid = request->xid;  // Reutilizamos el ID de transacción del cliente
    ack.flags = request->flags;  // Reutilizamos los flags del cliente
    memcpy(ack.chaddr, request->chaddr, 6);  // Copiamos solo la dirección MAC (6 bytes)

    // Configurar las direcciones IP
    ack.yiaddr = htonl(requested_ip);  // Dirección IP solicitada por el cliente (confirmada)
    ack.siaddr = htonl(server_ip);     // IP del servidor DHCP (esto lo debes definir previamente)

    // Agregar las opciones DHCP
    send_dhcp_options(&ack, DHCP_ACK, requested_ip);

    // Calcular el tamaño del paquete DHCP, incluyendo las opciones
    ssize_t packet_size = sizeof(struct dhcp_packet);  // 16 bytes de opciones agregadas

    // Enviar el paquete ACK al cliente
    ssize_t sent_bytes = sendto(sockfd, &ack, packet_size, 0, 
                                (struct sockaddr*)client_addr, sizeof(*client_addr));
    if (sent_bytes < 0) {
        perror("Error al enviar DHCP ACK");
    } else {
        printf("DHCP ACK enviado a %s confirmando la IP: %s\n", inet_ntoa(client_addr->sin_addr), int_to_ip(requested_ip));
    }
}

void send_dhcp_nak(int sockfd, struct sockaddr_in* client_addr, struct dhcp_packet* request) {
    struct dhcp_packet nak;

    // Limpiar la estructura
    memset(&nak, 0, sizeof(struct dhcp_packet));

    // Configurar los campos principales del paquete DHCP
    nak.op = 2;  // 2 = BOOTREPLY (respuesta del servidor)
    nak.htype = 1;  // 1 = Ethernet
    nak.hlen = 6;   // Longitud de la dirección de hardware (6 para MAC)
    nak.xid = request->xid;  // Reutilizamos el ID de transacción del cliente
    nak.flags = request->flags;  // Reutilizamos los flags del cliente
    memcpy(nak.chaddr, request->chaddr, 6);  // Copiamos solo la dirección MAC (6 bytes)

    // Agregar las opciones DHCP directamente en posiciones fijas
    nak.options[0] = 53;  // Código de la opción: DHCP Message Type
    nak.options[1] = 1;   // Longitud de la opción
    nak.options[2] = DHCP_NAK;  // DHCPNAK

    // Opción 54: Server Identifier (la IP del servidor DHCP)
    nak.options[3] = 54;  // Código de la opción
    nak.options[4] = 4;   // Longitud de la opción
    memcpy(&nak.options[5], &server_ip, 4);  // IP del servidor DHCP

    // Opción 255: Fin de las opciones DHCP
    nak.options[9] = 255;  // Fin de opciones

    // Calcular el tamaño total del paquete DHCP incluyendo las opciones
    ssize_t packet_size = sizeof(struct dhcp_packet) - sizeof(nak.options) + 10;

    // Enviar el paquete NAK al cliente
    ssize_t sent_bytes = sendto(sockfd, &nak, packet_size, 0, 
                                (struct sockaddr*)client_addr, sizeof(*client_addr));
    if (sent_bytes < 0) {
        perror("Error al enviar DHCP NAK");
    } else {
        printf("DHCP NAK enviado a %s\n", inet_ntoa(client_addr->sin_addr));
    }
}

void initialize_ip_pool(ip_range_t* range, const char* start_ip, const char* end_ip, int pool_id) { 
    // Convertir las IPs de cadena a enteros (IPv4) 
    range->start_ip = ip_to_int(start_ip);
    range->end_ip = ip_to_int(end_ip);
    
    // Validar conversión de IPs
    if (range->start_ip == (uint32_t)-1 || range->end_ip == (uint32_t)-1) {
        fprintf(stderr, "Error: Falló la conversión de las direcciones IP de cadena a entero.\n");
        exit(EXIT_FAILURE);  // Alternativamente, se puede devolver un error en lugar de salir
    }

    range->pool_id = pool_id;

    // Validar que el rango sea correcto
    if (range->start_ip > range->end_ip) { 
        fprintf(stderr, "Error: El rango de IPs no es válido. IP de inicio mayor que la IP de fin.\n"); 
        exit(EXIT_FAILURE);  // Alternativamente, devolver un código de error 
    }

    // Limitar el rango de IPs a MAX_IPS si excede el tamaño máximo
    uint32_t total_ips_in_range = range->end_ip - range->start_ip + 1;
    if (total_ips_in_range > MAX_IPS) {
        range->end_ip = range->start_ip + MAX_IPS - 1;
    }

    // Mensaje de depuración 
    printf("Rango de IPs inicializado: %s - %s (%u - %u)\n", start_ip, end_ip, range->start_ip, range->end_ip); 
}

uint32_t ip_to_int(const char* ip_str) { 
    struct in_addr ip_addr;

    // Intentar convertir la IP de cadena a formato binario
    if (inet_aton(ip_str, &ip_addr) == 0) {
        fprintf(stderr, "Error: La dirección IP '%s' no es válida.\n", ip_str);
        return (uint32_t)-1;  // Retornar un valor de error si la conversión falla
    }

    // Convertir de orden de red a orden de host
    return ntohl(ip_addr.s_addr);
}

char* int_to_ip(uint32_t ip_int) {
    struct in_addr ip_addr;
    ip_addr.s_addr = htonl(ip_int);  // Convertir a formato de red

    // Convertir la dirección IP a una cadena
    char* ip_str = inet_ntoa(ip_addr);

    // Retornar una copia de la cadena para evitar sobrescritura de inet_ntoa
    return strdup(ip_str);  // Duplicar la cadena para tener memoria propia
}

uint32_t assign_ip_address(ip_range_t* range, struct dhcp_packet* request) {    
    pthread_mutex_lock(&ip_assignment_mutex);  // Bloquear el acceso al árbol de asignaciones

    // Iniciar desde la última IP asignada o desde el inicio del rango
    if (last_assigned_ip < range->start_ip || last_assigned_ip > range->end_ip) {
        last_assigned_ip = range->start_ip;  // Establecer dentro del rango de IPs
    }

    uint32_t potential_ip = last_assigned_ip;
    int attempts = 0;
    int max_attempts = range->end_ip - range->start_ip + 1;

    // Intentar encontrar la siguiente IP disponible en el rango
    do {
        if (!find_ip_assignment(ip_assignment_root, potential_ip)) {
            // La IP no está asignada, podemos usarla
            ip_assignment_root = insert_ip_assignment(ip_assignment_root, potential_ip, request->chaddr, default_lease_time);
            last_assigned_ip = potential_ip + 1;  // Actualizar `last_assigned_ip` para el próximo cliente
            if (last_assigned_ip > range->end_ip) {
                last_assigned_ip = range->start_ip;  // Reiniciar el ciclo de IPs en el rango
            }

            printf("Dirección IP asignada a cliente con MAC %02x:%02x:%02x:%02x:%02x:%02x: %s\n",
                   request->chaddr[0], request->chaddr[1], request->chaddr[2],
                   request->chaddr[3], request->chaddr[4], request->chaddr[5],
                   int_to_ip(potential_ip));
            pthread_mutex_unlock(&ip_assignment_mutex);  // Desbloquear antes de retornar
            return potential_ip;
        }

        // Avanzar a la siguiente IP
        potential_ip++;
        if (potential_ip > range->end_ip) {
            potential_ip = range->start_ip;  // Reiniciar desde el inicio del rango
        }

        attempts++;
    } while (attempts < max_attempts);  // Intentar hasta cubrir todo el rango

    pthread_mutex_unlock(&ip_assignment_mutex);  // Liberar el mutex si no se encuentra IP

    // Si llegamos aquí, no hay IPs disponibles
    fprintf(stderr, "Error: No hay más direcciones IP disponibles en el rango.\n");
    return 0;
}

ip_assignment_node_t* insert_ip_assignment(ip_assignment_node_t* root, uint32_t ip, uint8_t* mac, int lease_time) {
    ip_assignment_node_t* new_node = (ip_assignment_node_t*)malloc(sizeof(ip_assignment_node_t));
    if (new_node == NULL) {
        fprintf(stderr, "Error: No se pudo asignar memoria para el nuevo nodo de IP.\n");
        return root;
    }

    new_node->ip = ip;
    memcpy(new_node->mac, mac, 6);  // Copiar la dirección MAC
    new_node->lease_start = time(NULL);  // Tiempo de concesión actual
    new_node->lease_time = lease_time;
    new_node->left = new_node->right = NULL;

    // Insertar el nuevo nodo en el árbol de asignaciones
    if (root == NULL) {
        return new_node;
    }

    if (ip < root->ip) {
        root->left = insert_ip_assignment(root->left, ip, mac, lease_time);
    } else if (ip > root->ip) {
        root->right = insert_ip_assignment(root->right, ip, mac, lease_time);
    } else {
        fprintf(stderr, "Advertencia: La IP %u ya está asignada.\n", ip);
    }

    return root;
}

ip_assignment_node_t* delete_ip_assignment(ip_assignment_node_t* root, uint32_t ip) {
    pthread_mutex_lock(&ip_assignment_mutex);  // Bloquear acceso al árbol de asignaciones

    if (root == NULL) {
        pthread_mutex_unlock(&ip_assignment_mutex);
        return NULL;
    }

    // Recorrer el árbol para encontrar la IP
    if (ip < root->ip) {
        root->left = delete_ip_assignment(root->left, ip);
    } else if (ip > root->ip) {
        root->right = delete_ip_assignment(root->right, ip);
    } else {
        // Nodo encontrado, eliminar
        if (root->left == NULL) {
            ip_assignment_node_t* temp = root->right;
            free(root);
            pthread_mutex_unlock(&ip_assignment_mutex);  // Desbloquear antes de retornar
            return temp;
        } else if (root->right == NULL) {
            ip_assignment_node_t* temp = root->left;
            free(root);
            pthread_mutex_unlock(&ip_assignment_mutex);  // Desbloquear antes de retornar
            return temp;
        }

        // Nodo con dos hijos: encontrar el sucesor más pequeño (mínimo en el subárbol derecho)
        ip_assignment_node_t* temp = find_min(root->right);
        root->ip = temp->ip;
        memcpy(root->mac, temp->mac, 6);
        root->lease_start = temp->lease_start;
        root->lease_time = temp->lease_time;
        root->right = delete_ip_assignment(root->right, temp->ip);
    }

    pthread_mutex_unlock(&ip_assignment_mutex);  // Desbloquear el acceso
    return root;
}

ip_assignment_node_t* find_min(ip_assignment_node_t* node) {
    if (node == NULL) {
        return NULL;  // Árbol vacío o subárbol vacío
    }

    // Iterar hacia la izquierda hasta encontrar el nodo más pequeño
    while (node->left != NULL) {
        node = node->left;
    }
    return node;
}

ip_assignment_node_t* find_ip_assignment(ip_assignment_node_t* root, uint32_t ip) {
    pthread_mutex_unlock(&ip_assignment_mutex);  // Desbloquear antes de retornar
    pthread_mutex_lock(&ip_assignment_mutex);  // Bloquear acceso al árbol de asignaciones
    
    while (root != NULL) {
        if (ip == root->ip) {
            pthread_mutex_unlock(&ip_assignment_mutex);  // Desbloquear antes de retornar
            return root;
        } else if (ip < root->ip) {
            root = root->left;
        } else {
            root = root->right;
        }
    }

    pthread_mutex_unlock(&ip_assignment_mutex);  // Desbloquear si no se encuentra la IP
    return NULL;
}

void free_ip_assignment_tree(ip_assignment_node_t* root) {
    if (root == NULL) return;
    free_ip_assignment_tree(root->left);
    free_ip_assignment_tree(root->right);
    free(root);
}

ip_assignment_node_t* check_expired_leases(ip_assignment_node_t* root) {
    if (root == NULL) return NULL;
    time_t current_time = time(NULL);

    // Primero recorrer el subárbol izquierdo y derecho
    root->left = check_expired_leases(root->left);
    root->right = check_expired_leases(root->right);

    // Luego verificar si el lease ha expirado
    if (current_time - root->lease_start > root->lease_time) {
        printf("El lease para la IP %s ha expirado.\n", int_to_ip(root->ip));
        root = delete_ip_assignment(root, root->ip);  // Eliminar el nodo del árbol
    }

    return root;  // Retornar la nueva raíz después de posibles eliminaciones
}

void send_dhcp_options(struct dhcp_packet* packet, int message_type, uint32_t assigned_ip) {
    // Reiniciar opciones
    memset(packet->options, 0, sizeof(packet->options));

    // Opción 53: Tipo de mensaje DHCP
    packet->options[0] = 53;   // Código de la opción
    packet->options[1] = 1;    // Longitud de la opción
    packet->options[2] = message_type;  // Tipo de mensaje DHCP

    // Opción 54: Identificador del servidor DHCP
    packet->options[3] = 54;   // Código de la opción
    packet->options[4] = 4;    // Longitud de la opción
    uint32_t net_server_ip = htonl(server_ip);
    memcpy(&packet->options[5], &net_server_ip, 4);

    // Opción 51: Tiempo de concesión
    packet->options[9] = 51;  // Código de la opción
    packet->options[10] = 4;  // Longitud de la opción
    uint32_t net_lease_time = htonl(default_lease_time);
    memcpy(&packet->options[11], &net_lease_time, 4);

    // Opción 1: Máscara de subred
    packet->options[15] = 1;   // Código de la opción
    packet->options[16] = 4;   // Longitud de la opción
    uint32_t net_subnet_mask = htonl(subnet_mask);
    memcpy(&packet->options[17], &net_subnet_mask, 4);

    // Opción 3: Puerta de enlace (Gateway)
    packet->options[21] = 3;   // Código de la opción
    packet->options[22] = 4;   // Longitud de la opción
    uint32_t net_gateway_ip = htonl(gateway_ip);
    memcpy(&packet->options[23], &net_gateway_ip, 4);

    // Opción 6: Servidor DNS
    packet->options[27] = 6;   // Código de la opción
    packet->options[28] = 4;   // Longitud de la opción
    uint32_t net_dns_server_ip = htonl(dns_server_ip);
    memcpy(&packet->options[29], &net_dns_server_ip, 4);

    // Opción 12: Nombre de host
    const char* hostname = "DHCPClient";
    packet->options[33] = 12;  // Código de la opción
    packet->options[34] = strlen(hostname);  // Longitud de la opción
    memcpy(&packet->options[35], hostname, strlen(hostname));

    // Opción 15: Nombre de dominio
    const char* domain_name = "example.com";
    packet->options[35 + strlen(hostname)] = 15;  // Código de la opción
    packet->options[36 + strlen(hostname)] = strlen(domain_name);  // Longitud de la opción
    memcpy(&packet->options[37 + strlen(hostname)], domain_name, strlen(domain_name));

    // Opción 55: Lista de parámetros solicitados
    uint8_t parameter_request_list[] = {1, 3, 6, 12, 15, 28, 42, 51, 54, 119};
    int param_request_offset = 37 + strlen(hostname) + strlen(domain_name);
    packet->options[param_request_offset] = 55;  // Código de la opción
    packet->options[param_request_offset + 1] = sizeof(parameter_request_list);  // Longitud
    memcpy(&packet->options[param_request_offset + 2], parameter_request_list, sizeof(parameter_request_list));

    // Opción 28: Dirección de broadcast
    uint32_t broadcast_addr = (assigned_ip & subnet_mask) | ~subnet_mask;
    broadcast_addr = htonl(broadcast_addr);
    int broadcast_offset = param_request_offset + 2 + sizeof(parameter_request_list);
    packet->options[broadcast_offset] = 28;  // Código de la opción
    packet->options[broadcast_offset + 1] = 4;  // Longitud
    memcpy(&packet->options[broadcast_offset + 2], &broadcast_addr, 4);

    // Opción 42: Servidor NTP
    uint32_t ntp_server = inet_addr("192.168.1.2");
    packet->options[broadcast_offset + 6] = 42;  // Código de la opción
    packet->options[broadcast_offset + 7] = 4;  // Longitud
    memcpy(&packet->options[broadcast_offset + 8], &ntp_server, 4);

    // Opción 255: Fin de las opciones
    packet->options[broadcast_offset + 12] = 255;  // Código de fin de opciones
}

void handle_signal(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        printf("\nSeñal %d recibida. Cerrando el servidor DHCP...\n", signal);

        // Cerrar el socket del servidor
        if (server_socket > 0) {
            if (close(server_socket) < 0) {
                perror("Error al cerrar el socket del servidor");
            } else {
                printf("Socket del servidor cerrado correctamente.\n");
            }
        }

        // Liberar la memoria del árbol de asignaciones de IPs
        if (ip_assignment_root != NULL) {
            free_ip_assignment_tree(ip_assignment_root);
            printf("Memoria liberada para el árbol de asignaciones de IPs.\n");
        }

        // Destruir los mutex (si se están utilizando)
        if (pthread_mutex_destroy(&ip_assignment_mutex) != 0) {
            perror("Error al destruir el mutex");
        } else {
            printf("Mutex destruidos.\n");
        }

        // Imprimir mensaje de cierre final
        printf("Servidor DHCP cerrado correctamente.\n");

        // Salir del programa
        exit(0);
    }
}

uint8_t* find_dhcp_option(uint8_t* options, uint8_t code) {
    int i = 0;

    // Recorrer las opciones hasta encontrar la opción de fin (código 255)
    while (i < 312) {  // Longitud fija del array de opciones
        uint8_t option_code = options[i];  // Código de la opción actual

        // Si encontramos el código de fin (255), detener la búsqueda
        if (option_code == 255) {
            printf("End of options reached.\n");
            break;
        }

        // Si encontramos el código de opción deseado, devolver un puntero al valor de la opción
        if (option_code == code) {
            return &options[i + 2];  // Retornar el puntero al valor (i + 2 salta código y longitud)
        }

        // Si no es la opción deseada, avanzar a la siguiente (código + longitud + valor)
        uint8_t option_length = options[i + 1];  // Longitud de la opción
        i += 2 + option_length;  // Avanzar al siguiente código de opción
    }

    // Si no se encuentra la opción, devolver NULL
    printf("Option %d not found.\n", code);
    return NULL;
}

int get_lease_remaining(ip_assignment_node_t* assignment) {
    if (assignment == NULL) {
        return -1; // Retorna -1 si no hay asignación válida
    }

    // Validar que los tiempos sean sensatos (lease_time no negativo y lease_start válido)
    if (assignment->lease_time <= 0 || assignment->lease_start <= 0) {
        return -1; // Valores inválidos
    }

    time_t current_time = time(NULL); // Obtener el tiempo actual
    int elapsed_time = current_time - assignment->lease_start; // Tiempo transcurrido desde el inicio del lease

    int remaining_time = assignment->lease_time - elapsed_time; // Calcular el tiempo restante

    // Si el tiempo restante es negativo, significa que el lease ha expirado
    return remaining_time > 0 ? remaining_time : 0; // Si es negativo, devolver 0
}

void print_tree(ip_assignment_node_t* root) {
    if (root == NULL) {
        return;  // Si el nodo es NULL, no hacemos nada
    }

    // Recorrer el subárbol izquierdo
    print_tree(root->left);

    // Obtener la representación de la IP
    char* ip_str = int_to_ip(root->ip);

    // Imprimir la información del nodo actual
    printf("IP: %s, MAC: %02x:%02x:%02x:%02x:%02x:%02x, Lease Time: %d, Remaining: %d\n",
           ip_str,
           root->mac[0], root->mac[1], root->mac[2],
           root->mac[3], root->mac[4], root->mac[5],
           root->lease_time, get_lease_remaining(root));

    // Liberar la memoria de la cadena IP (si `int_to_ip` asigna dinámicamente)
    free(ip_str);

    // Recorrer el subárbol derecho
    print_tree(root->right);
}

// Función para limpiar recursos y salir del programa
void cleanup() {
    if (server_socket != -1) close(server_socket);
    pthread_mutex_destroy(&ip_assignment_mutex);
    pthread_mutex_destroy(&client_id_mutex);
}

// Buscar si ya existe un cliente con el mismo XID o chaddr
client_thread_info_t* find_client_thread(uint8_t* chaddr) {
// Recorrer toda la tabla hash para buscar el cliente basado en la dirección MAC
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        client_thread_info_t* current = client_threads[i];
        while (current != NULL) {
            // Comparamos solo la MAC address
            if (memcmp(current->chaddr, chaddr, 6) == 0) {
                return current;  // Cliente encontrado
            }
            current = current->next;
        }
    }
    return NULL;  // No se encontró cliente con esa MAC
}

// Añadir un cliente nuevo a la tabla hash
void add_client_thread(uint8_t* chaddr, pthread_t thread_id, client_info_t* client_info) {
    // Calcular el índice hash usando la dirección MAC del cliente
    unsigned int hash = hash_mac(chaddr);

    // Crear una nueva entrada para el cliente
    client_thread_info_t* new_entry = (client_thread_info_t*)malloc(sizeof(client_thread_info_t));
    if (new_entry == NULL) {
        perror("Error al asignar memoria para la nueva entrada de cliente");
        return;
    }

    memcpy(new_entry->chaddr, chaddr, 6);  // Copiar la dirección MAC
    new_entry->thread_id = thread_id;
    new_entry->client_info = client_info;

    // Insertar al principio de la lista enlazada en la posición hash calculada
    new_entry->next = client_threads[hash];
    client_threads[hash] = new_entry;
}

// Eliminar un cliente de la tabla hash
void remove_client_thread(uint8_t* chaddr) {
    for (int i = 0; i < HASH_TABLE_SIZE; ++i) {
        client_thread_info_t* current = client_threads[i];
        client_thread_info_t* prev = NULL;

        // Recorrer la lista enlazada buscando por dirección MAC
        while (current != NULL) {
            if (memcmp(current->chaddr, chaddr, 6) == 0) {
                // Encontramos el cliente, eliminamos el nodo
                if (prev == NULL) {
                    // El cliente está al principio de la lista
                    client_threads[i] = current->next;
                } else {
                    // El cliente está en el medio o al final de la lista
                    prev->next = current->next;
                }
                free(current);
                printf("Cliente con MAC %02x:%02x:%02x:%02x:%02x:%02x eliminado correctamente.\n",
                       chaddr[0], chaddr[1], chaddr[2], chaddr[3], chaddr[4], chaddr[5]);
                return;
            }
            prev = current;
            current = current->next;
        }
    }

    // Si llegamos aquí, el cliente no se encontró
    printf("Cliente con MAC %02x:%02x:%02x:%02x:%02x:%02x no encontrado para eliminación.\n",
           chaddr[0], chaddr[1], chaddr[2], chaddr[3], chaddr[4], chaddr[5]);
    exit(1);
}

unsigned int hash_mac(const uint8_t* mac) {
    unsigned int hash = 0;
    for (int i = 0; i < 6; ++i) {
        hash = (hash * 31) + mac[i];  // Mezclar cada byte de la MAC en el hash
    }
    return hash % HASH_TABLE_SIZE;  // Asegurar que el hash esté dentro del rango de la tabla
}
