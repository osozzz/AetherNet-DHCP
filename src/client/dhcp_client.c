#include "dhcp_client.h"

// Función que inicia el cliente DHCP
void init_dhcp_client() {

    struct sockaddr_in relay_addr, client_addr;
    dhcp_client_t client;

    // Inicializar el cliente
    client.nak_attempts = 0;
    client.decline_attempts = 0;

    // Inicializar el generador de números aleatorios para `xid` y MAC aleatoria
    srand(time(NULL));  // Semilla aleatoria basada en el tiempo actual

    generate_random_mac(client.mac_addr);
    printf("MAC Address generada para el cliente: %02X:%02X:%02X:%02X:%02X:%02X\n",
           client.mac_addr[0], client.mac_addr[1], client.mac_addr[2],
           client.mac_addr[3], client.mac_addr[4], client.mac_addr[5]);

    // Crear el socket UDP para el cliente con puerto dinámico
    client.sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (client.sockfd < 0) {
        perror("Error al crear el socket del cliente DHCP");
        exit(EXIT_FAILURE);
    }
    printf("Socket del cliente DHCP creado exitosamente.\n");

    // Habilitar el broadcast en el socket
    int broadcast_enable = 1;
    if (setsockopt(client.sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(broadcast_enable)) < 0) {
        perror("Error al habilitar broadcast en el socket del cliente DHCP");
        close(client.sockfd);
        exit(EXIT_FAILURE);
    }

    // Configurar dirección del cliente
    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = 0;
    client_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(client.sockfd, (struct sockaddr*)&client_addr, sizeof(client_addr)) < 0) {
        perror("Error al hacer bind en el socket del cliente DHCP");
        close(client.sockfd);
        exit(EXIT_FAILURE);
    }

    // Obtener y mostrar el puerto asignado al cliente
    socklen_t addr_len = sizeof(client_addr);
    if (getsockname(client.sockfd, (struct sockaddr*)&client_addr, &addr_len) == -1) {
        perror("Error al obtener la dirección del socket del cliente");
    } else {
        printf("Puerto asignado al cliente: %d\n", ntohs(client_addr.sin_port));
    }

    // Configurar la dirección del relay DHCP (en lugar del servidor DHCP)
    memset(&relay_addr, 0, sizeof(relay_addr));
    relay_addr.sin_family = AF_INET;
    relay_addr.sin_port = htons(68);  // Puerto del relay DHCP (1067)
    relay_addr.sin_addr.s_addr = INADDR_BROADCAST;  // Usar broadcast
    
    // Llamar a la función que maneja el ciclo DHCP, usando la dirección del relay
    handle_dhcp_protocol(&client, &relay_addr);
}

void handle_dhcp_protocol(dhcp_client_t* client, struct sockaddr_in* server_addr) {
    fd_set readfds;
    struct timeval timeout;
    int discovery_sent = 0;  // Variable para controlar si ya se envió DHCP Discover

    // Enviar el DHCPDISCOVER al relay una sola vez antes de entrar al ciclo
    send_dhcp_discover(client->sockfd, server_addr, client->mac_addr);
    discovery_sent = 1;

    while (1) {  // Ciclo infinito para manejar respuestas y reintentos
        // Preparar el descriptor de archivos para esperar la respuesta
        FD_ZERO(&readfds);
        FD_SET(client->sockfd, &readfds);

        timeout.tv_sec = 120;  // Esperar máximo 20 segundos por una oferta
        timeout.tv_usec = 0;

        int ready = select(client->sockfd + 1, &readfds, NULL, NULL, &timeout);
        if (ready < 0) {
            perror("Error en select()");
            close(client->sockfd);
            exit(EXIT_FAILURE);
        } else if (ready == 0) {
            // Si hay un timeout sin recibir respuesta
            printf("Timeout: No se recibió respuesta del servidor DHCP. Reintentando...\n");

            if (discovery_sent > 0 || client->nak_attempts > 1 || client->decline_attempts > 1) {
                // Volver a enviar DHCP Discover solo si aún no se ha enviado o tras NAK/Decline
                send_dhcp_discover(client->sockfd, server_addr, client->mac_addr);
                discovery_sent = 1;
            }

            continue;  // Reintentar el proceso
        }

        // Si hay datos disponibles en el socket, leer el paquete DHCP
        struct dhcp_packet offer_message;
        struct sockaddr_in recv_addr;
        socklen_t addr_len = sizeof(recv_addr);

        // Recibir el mensaje DHCP desde el relay o servidor
        int recv_len = recvfrom(client->sockfd, &offer_message, sizeof(offer_message), 0,
                                (struct sockaddr *)&recv_addr, &addr_len);
        if (recv_len < 0) {
            perror("Error al recibir el mensaje DHCP");
            continue;  // Reintentar si falla
        }

        // Verificar las opciones del mensaje para identificar el tipo
        uint8_t* message_type = find_dhcp_option(offer_message.options, 53);
        if (message_type) {
            switch (*message_type) {
                case DHCP_OFFER:
                    printf("Oferta DHCP recibida: %s\n", inet_ntoa(recv_addr.sin_addr));
                    handle_dhcp_offer(client, server_addr, &offer_message);  // Procesar la oferta
                    break;

                case DHCP_ACK:
                    printf("ACK recibido del servidor.\n");
                    handle_dhcp_ack(client, &offer_message, server_addr);  // Procesar el ACK
                    break;

                case DHCP_NAK:
                    printf("Recibido NAK del servidor. Reintentando...\n");
                    handle_dhcp_nak(client);  // Procesar el NAK
                    break;

                case DHCP_DECLINE:
                    printf("Recibido DECLINE. Reintentando...\n");
                    client->decline_attempts++;
                    if (client->decline_attempts > 1) {
                        printf("Máximo de reintentos de DECLINE alcanzado. Cerrando cliente DHCP.\n");
                        close(client->sockfd);
                        exit(EXIT_FAILURE);
                    }
                    // Enviar DHCP Discover si se alcanza el límite de reintentos de Decline
                    send_dhcp_discover(client->sockfd, server_addr, client->mac_addr);
                    discovery_sent = 1;
                    continue;

                default:
                    printf("Mensaje DHCP inesperado recibido. Reintentando...\n");
                    continue;
            }
        } else {
            printf("Paquete DHCP sin opción de tipo válida. Reintentando...\n");
            continue;
        }
    }
}

// Función que envía un DHCPDISCOVER al servidor
void send_dhcp_discover(int sockfd, struct sockaddr_in* server_addr, uint8_t* mac_addr) {
    struct dhcp_packet discover_packet;
    memset(&discover_packet, 0, sizeof(struct dhcp_packet));

    // Configurar los campos principales del paquete DHCPDISCOVER
    discover_packet.op = 1;  // 1 = solicitud
    discover_packet.htype = 1;  // Tipo de hardware: Ethernet
    discover_packet.hlen = 6;   // Longitud de la dirección de hardware (6 bytes para MAC)
    discover_packet.xid = htonl(random());  // Generar un ID de transacción aleatorio
    discover_packet.flags = htons(0x8000);  // Solicitar respuesta de broadcast

    // Configurar la dirección MAC del cliente (se puede personalizar)
    memcpy(discover_packet.chaddr, mac_addr, 6);

    // Opciones DHCP fijas
    discover_packet.options[0] = 53;  // Opción 53: DHCP Message Type
    discover_packet.options[1] = 1;   // Longitud de la opción
    discover_packet.options[2] = DHCP_DISCOVER;  // DHCPDISCOVER

    // Opción 55: Parameter Request List (Lista de parámetros solicitados)
    discover_packet.options[3] = 55;  // Opción 55: Parameter Request List
    discover_packet.options[4] = 3;   // Longitud de la opción
    discover_packet.options[5] = 1;   // Solicitar Subnet Mask
    discover_packet.options[6] = 3;   // Solicitar Router
    discover_packet.options[7] = 6;   // Solicitar DNS

    // Opción 255: Fin de las opciones
    discover_packet.options[8] = 255;

    // Calcular el tamaño del paquete real (base del paquete más las opciones)
    ssize_t packet_size = sizeof(struct dhcp_packet) - sizeof(discover_packet.options) + 9;

    // Enviar el paquete DISCOVER al servidor
    ssize_t sent_bytes = sendto(sockfd, &discover_packet, packet_size, 0,
                                (struct sockaddr*)server_addr, sizeof(*server_addr));
    if (sent_bytes < 0) {
        perror("Error al enviar el paquete DHCP DISCOVER");
    } else {
        printf("Paquete DHCP DISCOVER enviado.\n");
    }
}

// Función auxiliar para validar que la IP ofrecida está dentro de un rango válido
int validate_offered_ip(struct in_addr* offered_ip, struct in_addr* subnet_mask, struct in_addr* network_ip) {
    uint32_t offered = ntohl(offered_ip->s_addr);
    uint32_t mask = ntohl(subnet_mask->s_addr);
    uint32_t network = ntohl(network_ip->s_addr);

    // Aplicar la máscara de subred a la IP ofrecida y compararla con la red esperada
    if ((offered & mask) == (network & mask)) {
        printf("IP ofrecida (%s) válida y dentro de la red.\n", inet_ntoa(*offered_ip));
        return 1;  // La IP está dentro del rango válido
    } else {
        printf("Error: IP ofrecida (%s) fuera de la red.\n", inet_ntoa(*offered_ip));
        return 0;  // La IP está fuera del rango válido
    }
}

// Función que maneja la oferta DHCP (DHCPOFFER)
void handle_dhcp_offer(dhcp_client_t* client, struct sockaddr_in* server_addr, struct dhcp_packet* offer_packet) {
    struct in_addr offered_ip;
    offered_ip.s_addr = offer_packet->yiaddr;  // IP ofrecida por el servidor

    printf("Procesando la oferta DHCP. IP ofrecida: %s\n", inet_ntoa(offered_ip));

    // Validar la IP ofrecida utilizando la máscara de subred y la IP de red base
    if (validate_offered_ip(&offered_ip, &(client->subnet_mask), &(client->network_ip))) {
        // Si la validación es exitosa, enviar DHCPREQUEST
        printf("IP ofrecida válida. Enviando DHCPREQUEST para la IP: %s\n", inet_ntoa(offered_ip));
        send_dhcp_request(client->sockfd, server_addr, &offered_ip, client->mac_addr);
    } else {
        // Si la validación falla, enviar DHCPDECLINE
        printf("IP ofrecida inválida. Enviando DHCPDECLINE para la IP: %s\n", inet_ntoa(offered_ip));
        send_dhcp_decline(client->sockfd, &offered_ip, &(client->server_addr), client->mac_addr);
    }
}

// Función que envía un DHCPREQUEST al servidor
void send_dhcp_request(int sockfd, struct sockaddr_in* server_addr, struct in_addr* offered_ip, uint8_t* mac_addr) {
    struct dhcp_packet request_packet;
    memset(&request_packet, 0, sizeof(struct dhcp_packet));

    // Configurar los campos principales del paquete DHCPREQUEST
    request_packet.op = 1;  // 1 = solicitud
    request_packet.htype = 1;  // Tipo de hardware: Ethernet
    request_packet.hlen = 6;   // Longitud de la dirección de hardware (6 bytes para MAC)
    request_packet.xid = htonl(random());  // Generar un ID de transacción aleatorio
    request_packet.flags = htons(0x8000);  // Solicitar respuesta de broadcast

    // Configurar la dirección MAC del cliente (se puede personalizar
    memcpy(request_packet.chaddr, mac_addr, 6); // Luego copia los 6 bytes de la dirección MAC
    request_packet.ciaddr = offered_ip->s_addr;

    // Opciones DHCP fijas
    request_packet.options[0] = 53;  // Opción 53: DHCP Message Type
    request_packet.options[1] = 1;   // Longitud de la opción
    request_packet.options[2] = DHCP_REQUEST;  // DHCPREQUEST

    // Opción 50: IP solicitada
    request_packet.options[3] = 50;  // Opción 50: Requested IP Address
    request_packet.options[4] = 4;   // Longitud de la opción (4 bytes para dirección IP)
    memcpy(&request_packet.options[5], &offered_ip->s_addr, 4);  // Copiar la IP solicitada

    // Opción 54: Server Identifier (la IP del servidor DHCP)
    request_packet.options[9] = 54;  // Opción 54: Server Identifier
    request_packet.options[10] = 4;  // Longitud de la opción (4 bytes para dirección IP)
    memcpy(&request_packet.options[11], &server_addr->sin_addr.s_addr, 4);  // Copiar la IP del servidor

    // Opción 55: Parameter Request List (Lista de parámetros solicitados)
    request_packet.options[15] = 55;  // Opción 55: Parameter Request List
    request_packet.options[16] = 3;   // Longitud de la opción
    request_packet.options[17] = 1;   // Solicitar Subnet Mask
    request_packet.options[18] = 3;   // Solicitar Router
    request_packet.options[19] = 6;   // Solicitar DNS

    // Opción 255: Fin de las opciones
    request_packet.options[20] = 255;

    ssize_t packet_size = sizeof(struct dhcp_packet) - sizeof(request_packet.options) + 21;

    // Enviar el paquete REQUEST al servidor
    ssize_t sent_bytes = sendto(sockfd, &request_packet, packet_size, 0,
                                (struct sockaddr*)server_addr, sizeof(*server_addr));
    if (sent_bytes < 0) {
        perror("Error al enviar el paquete DHCPREQUEST");
    } else {
        printf("Paquete DHCPREQUEST enviado.\n");
    }
}

// Función que envía un DHCPDECLINE al servidor
void send_dhcp_decline(int sockfd, struct in_addr* offered_ip, struct sockaddr_in* server_addr, uint8_t* mac_addr) {
    struct dhcp_packet decline_packet;
    memset(&decline_packet, 0, sizeof(struct dhcp_packet));

    // Configurar los campos principales del paquete DHCPDECLINE
    decline_packet.op = 1;  // 1 = solicitud
    decline_packet.htype = 1;  // Tipo de hardware: Ethernet
    decline_packet.hlen = 6;   // Longitud de la dirección de hardware (6 bytes para MAC)
    decline_packet.xid = htonl(random());  // Generar un ID de transacción aleatorio
    decline_packet.flags = htons(0x8000);  // Solicitar respuesta de broadcast

    // Establecer la IP ofrecida como la IP rechazada (yiaddr)
    decline_packet.yiaddr = offered_ip->s_addr;

    // Configurar la dirección MAC del cliente (se puede personalizar)
    memcpy(&decline_packet.chaddr, mac_addr, 6);

    // Opciones DHCP fijas
    decline_packet.options[0] = 53;  // Opción 53: DHCP Message Type
    decline_packet.options[1] = 1;   // Longitud de la opción
    decline_packet.options[2] = DHCP_DECLINE;  // DHCPDECLINE

    // Opción 50: IP ofrecida que estamos rechazando
    decline_packet.options[3] = 50;  // Opción 50: Requested IP Address
    decline_packet.options[4] = 4;   // Longitud de la opción (4 bytes para dirección IP)
    memcpy(&decline_packet.options[5], &offered_ip->s_addr, 4);  // Copiar la IP ofrecida

    // Opción 54: Server Identifier (la IP del servidor DHCP)
    decline_packet.options[9] = 54;  // Opción 54: Server Identifier
    decline_packet.options[10] = 4;  // Longitud de la opción (4 bytes para dirección IP)
    memcpy(&decline_packet.options[11], &server_addr->sin_addr.s_addr, 4);  // Copiar la IP del servidor

    // Opción 255: Fin de las opciones
    decline_packet.options[15] = 255;

    // Calcular el tamaño del paquete real (base del paquete más las opciones)
    ssize_t packet_size = sizeof(struct dhcp_packet) - sizeof(decline_packet.options) + 16;  // 16 por las opciones añadidas

    // Enviar el paquete DECLINE al servidor
    ssize_t sent_bytes = sendto(sockfd, &decline_packet, packet_size, 0,
                                (struct sockaddr*)server_addr, sizeof(*server_addr));
    if (sent_bytes < 0) {
        perror("Error al enviar el paquete DHCPDECLINE");
    } else {
        printf("Paquete DHCPDECLINE enviado.\n");
    }
}

// Función que maneja el DHCPNAK (rechazo de solicitud de IP o renovación)
void handle_dhcp_nak(dhcp_client_t* client) {
    printf("Procesando DHCP NAK del servidor...\n");

    // Abandonar la IP ofrecida
    printf("La solicitud de IP %s ha sido rechazada por el servidor.\n", inet_ntoa(client->offered_ip));
    client->offered_ip.s_addr = 0;  // Borrar la IP ofrecida

    // Comprobar el estado de los intentos de NAK
    client->nak_attempts++;
    if (client->nak_attempts > 1) {
        printf("Máximo de reintentos de NAK alcanzado. Cerrando cliente DHCP.\n");
        close(client->sockfd);
        exit(EXIT_FAILURE);
    }

    // Reintentar el proceso desde el principio: enviar DHCP Discover de nuevo
    printf("Reintentando el proceso de DHCP Discover...\n");
    send_dhcp_discover(client->sockfd, &(client->server_addr), client->mac_addr);
}

// Función que maneja el DHCPACK (confirmación de asignación de IP)
void handle_dhcp_ack(dhcp_client_t* client, struct dhcp_packet* ack_packet, struct sockaddr_in* server_addr) {
    printf("Procesando DHCP ACK del servidor...\n");

    // Extraer la IP asignada (yiaddr)
    client->offered_ip.s_addr = ack_packet->yiaddr;
    printf("IP asignada por el servidor: %s\n", inet_ntoa(client->offered_ip));

    // Extraer las opciones importantes del ACK
    // Extraer las opciones importantes del ACK (Submask, Router, DNS, Lease Time)
    uint8_t* subnet_mask_option = find_dhcp_option(ack_packet->options, 1);  // Opción 1: Subnet Mask
    uint8_t* router_option = find_dhcp_option(ack_packet->options, 3);       // Opción 3: Gateway (router)
    uint8_t* dns_option = find_dhcp_option(ack_packet->options, 6);          // Opción 6: DNS
    uint8_t* lease_time_option = find_dhcp_option(ack_packet->options, 51);  // Opción 51: Lease Time

    // Procesar la máscara de subred
    if (subnet_mask_option) {
        memcpy(&(client->subnet_mask.s_addr), subnet_mask_option, 4);
        printf("Máscara de subred asignada: %s\n", inet_ntoa(client->subnet_mask));
    } else {
        printf("No se recibió la opción de Máscara de subred.\n");
    }

    // Procesar el Gateway (router)
    struct in_addr router_ip;
    if (router_option) {
        memcpy(&(router_ip.s_addr), router_option, 4);
        printf("Puerta de enlace (Gateway) asignada: %s\n", inet_ntoa(router_ip));
    } else {
        printf("No se recibió la opción de Puerta de enlace.\n");
    }

    // Procesar los servidores DNS
    struct in_addr dns_ip;
    if (dns_option) {
        memcpy(&(dns_ip.s_addr), dns_option, 4);
        printf("Servidor DNS asignado: %s\n", inet_ntoa(dns_ip));
    } else {
        printf("No se recibió la opción de Servidor DNS.\n");
    }

    // Procesar el tiempo de concesión (lease time)
    if (lease_time_option) {
        client->lease_time = ntohl(*(uint32_t*)lease_time_option);  // Convertir desde formato de red a host
        printf("Tiempo de concesión (lease time): %u segundos\n", client->lease_time);

        // Calcular tiempos de renovación (T1) y rebind (T2)
        client->renewal_time = client->lease_time / 2;      // T1: A la mitad del lease time
        client->rebind_time = (client->lease_time * 7) / 8; // T2: A 7/8 del lease time
        printf("Tiempo para renovación (T1): %u segundos\n", client->renewal_time);
        printf("Tiempo para rebind (T2): %u segundos\n", client->rebind_time);
    } else {
        printf("No se recibió la opción de Tiempo de concesión (lease time).\n");
    }
    // Llamar a la función de manejo del ciclo de renovación del lease
    handle_lease_renewal(client, server_addr, &client->offered_ip);
}

// Función para manejar la renovación del lease (T1 y T2)
void handle_lease_renewal(dhcp_client_t* client, struct sockaddr_in* server_addr, struct in_addr* offered_ip) {
    struct timeval current_time, start_time;
    gettimeofday(&start_time, NULL);  // Obtener el tiempo actual cuando se asigna el lease
    int t1_count = 0;
    int t2_count = 0;

    while (1) {
        gettimeofday(&current_time, NULL);  // Obtener el tiempo actual en cada ciclo
        long elapsed_time = current_time.tv_sec - start_time.tv_sec;

        if (elapsed_time >= client->lease_time) {
            printf("Lease expirado. Enviando Release y Cerrando cliente DHCP.\n");
            send_dhcp_release(client, server_addr, offered_ip);
            close(client->sockfd);
            exit(EXIT_FAILURE);  // Terminar el proceso cuando el lease expira
        }

        if (elapsed_time >= client->rebind_time && t2_count < 1) {
            // T2 alcanzado: enviar un DHCP Request en broadcast (rebind)
            printf("Tiempo T2 alcanzado. Enviando DHCP Request en broadcast (rebind).\n");
            struct sockaddr_in broadcast_addr;
            memset(&broadcast_addr, 0, sizeof(broadcast_addr));
            broadcast_addr.sin_family = AF_INET;
            broadcast_addr.sin_port = htons(67);  // Puerto del servidor DHCP (67)
            broadcast_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);  // Usar broadcast

            send_dhcp_request(client->sockfd, &broadcast_addr, &(client->offered_ip), client->mac_addr);  // Rebind en broadcast
            t2_count++;
        } else if (elapsed_time >= client->renewal_time && t1_count < 1) {
            // T1 alcanzado: enviar un DHCP Request directamente al servidor (renovación)
            printf("Tiempo T1 alcanzado. Enviando DHCP Request al servidor para renovación.\n");
            send_dhcp_request(client->sockfd, server_addr, offered_ip, client->mac_addr);  // Renovación (T1)
            t1_count++;
        }

        // Usar select para esperar de forma más eficiente
        struct timeval wait_time;
        long wait_seconds = client->lease_time - elapsed_time;
        wait_time.tv_sec = (wait_seconds > 3) ? 3 : wait_seconds;
        wait_time.tv_usec = 0;

        select(0, NULL, NULL, NULL, &wait_time);  // Suspender el ciclo durante `wait_time` segundos
    }
}

// Función para enviar un mensaje DHCP Release
void send_dhcp_release(dhcp_client_t* client, struct sockaddr_in* server_addr, struct in_addr* offered_ip) {
    struct dhcp_packet release_packet;
    memset(&release_packet, 0, sizeof(struct dhcp_packet));

    // Configurar los campos principales del paquete DHCPRELEASE
    release_packet.op = 1;  // 1 = solicitud
    release_packet.htype = 1;  // Tipo de hardware: Ethernet
    release_packet.hlen = 6;   // Longitud de la dirección de hardware (6 bytes para MAC)
    release_packet.xid = htonl(random());  // Identificador de transacción aleatorio
    release_packet.flags = htons(0x8000);  // Bandera de fin de opciones

    // Configurar la direccion MAC del cliente
    memcpy(release_packet.chaddr, client->mac_addr, 6);
    // Configurar la dirección IP del servidor DHCP
    release_packet.ciaddr = offered_ip->s_addr;

    // Opciones DHCP Fijas
    release_packet.options[0] = 53;  // Opción 53: Tipo de mensaje DHCP
    release_packet.options[1] = 1;   // Longitud de la opción
    release_packet.options[2] = DHCP_RELEASE;  // DHCPRELEASE

    // Opción de fin (código 255)
    release_packet.options[3] = 255;   // Longitud de la opción

    ssize_t packet_size = sizeof(struct dhcp_packet) - sizeof(release_packet.options) + 4;

    // Enviar el paquete REQUEST al servidor
    ssize_t sent_bytes = sendto(client->sockfd, &release_packet, packet_size, 0,
                                (struct sockaddr*)server_addr, sizeof(*server_addr));
    if (sent_bytes < 0) {
        perror("Error al enviar el paquete DHCPRELEASE");
    } else {
        printf("Paquete DHCPRELEASE enviado.\n");
    }
}

uint8_t* find_dhcp_option(uint8_t* options, uint8_t code) {
    int i = 0;

    // Recorrer las opciones hasta encontrar la opción de fin (código 255)
    while (i < 312) {  // Longitud fija del array de opciones
        uint8_t option_code = options[i];  // Código de la opción actual

        // Si encontramos el código de fin (255), detener la búsqueda
        if (option_code == 255) {
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
    return NULL;
}

// Función para generar una MAC Address aleatoria
void generate_random_mac(uint8_t* mac_addr) {

    // Generar los primeros 3 bytes aleatorios
    mac_addr[0] = 0x02;  // Establecer unicast (bit 0 es 0) y localmente administrada (bit 1 es 1)
    mac_addr[1] = rand() % 256;
    mac_addr[2] = rand() % 256;

    // Generar los siguientes 3 bytes aleatorios
    mac_addr[3] = rand() % 256;
    mac_addr[4] = rand() % 256;
    mac_addr[5] = rand() % 256;
}