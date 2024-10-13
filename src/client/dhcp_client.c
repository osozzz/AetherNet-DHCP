#include "dhcp_client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

// Función para enviar un mensaje DHCP Discover
void send_dhcp_discover(int sockfd, struct sockaddr_in *server_addr) {
    struct dhcp_packet discover;
    memset(&discover, 0, sizeof(discover));

    // Rellenar los campos del mensaje DHCP Discover
    discover.op = DHCP_DISCOVER;   // Tipo de mensaje: Discover
    discover.htype = 1;            // Tipo de hardware: Ethernet
    discover.hlen = 6;             // Longitud de la dirección MAC
    discover.xid = htonl(123456789); // ID de transacción
    discover.flags = htons(0x8000);  // Bandera para usar broadcast

    // Enviar el paquete DHCP Discover al servidor
    ssize_t sent_len = sendto(sockfd, &discover, sizeof(discover), 0, (struct sockaddr *)server_addr, sizeof(*server_addr));
    if (sent_len < 0) {
        perror("Error enviando DHCP Discover");
        exit(EXIT_FAILURE);
    }
    printf("DHCP Discover enviado.\n");
}

// Función para recibir un mensaje DHCP Offer del servidor
void receive_dhcp_offer(int sockfd) {
    struct dhcp_packet offer;
    struct sockaddr_in server_addr;
    socklen_t server_addr_len = sizeof(server_addr);
    char buffer[BUFFER_SIZE];

    // Recibir un paquete DHCP Offer del servidor
    ssize_t received_len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&server_addr, &server_addr_len);
    if (received_len < 0) {
        perror("Error recibiendo DHCP Offer");
        exit(EXIT_FAILURE);
    }

    // Copiar los datos al paquete DHCP Offer
    memcpy(&offer, buffer, sizeof(offer));

    // Mostrar la dirección IP ofrecida
    struct in_addr yiaddr;
    yiaddr.s_addr = offer.yiaddr;
    printf("DHCP Offer recibido. IP ofrecida: %s\n", inet_ntoa(yiaddr));
}
