#include "dhcp_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>

// Función para inicializar el servidor DHCP
void init_dhcp_server() {
    int sockfd;
    struct sockaddr_in server_addr;

    // 1. Crear el socket UDP
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Error al crear el socket");
        exit(EXIT_FAILURE);
    }
    printf("Socket creado exitosamente.\n");

    // 2. Configurar la dirección del servidor
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // Escuchar en todas las interfaces
    server_addr.sin_port = htons(DHCP_SERVER_PORT);  // Puerto 67

    // 3. Vincular el socket al puerto
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error al vincular el socket");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    printf("Socket vinculado exitosamente al puerto %d.\n", DHCP_SERVER_PORT);

    // Llamar a la función para manejar solicitudes DHCP
    handle_dhcp_requests(sockfd);
    
    // Cerrar el socket después de terminar (aunque no se cerrará por el ciclo infinito)
    close(sockfd);
}

// Función para manejar las solicitudes DHCP entrantes
void handle_dhcp_requests(int sockfd) {
    struct sockaddr_in client_addr;
    char buffer[BUFFER_SIZE];
    socklen_t client_addr_len = sizeof(client_addr);

    // Bucle infinito para escuchar solicitudes
    while (1) {
        printf("Esperando solicitudes DHCP...\n");

        // Recibir un mensaje del cliente
        ssize_t received_len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, &client_addr_len);
        if (received_len < 0) {
            perror("Error al recibir el mensaje");
            continue;
        }

        // Imprimir información del cliente
        printf("Solicitud DHCP recibida de %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        
        // Aquí se analizará y manejará la solicitud DHCP (DHCP Discover, etc.)
    }
}
