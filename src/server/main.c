#include "dhcp_server.h"

int main() {
    // Configurar el manejo de señales
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    
    // Leer rango de IPs desde variables de entorno
    const char *start_ip = getenv("START_IP");
    const char *end_ip = getenv("END_IP");
    if (start_ip == NULL || end_ip == NULL) {
        fprintf(stderr, "Error: Las variables de entorno START_IP y END_IP son necesarias.\n");
        return 1;
    }

    // Configurar el rango de IPs
    ip_range_t range;
    initialize_ip_pool(&range, start_ip, end_ip, 1);

    // Inicializar el servidor DHCP
    init_dhcp_server(&range);
    return 0;
}
