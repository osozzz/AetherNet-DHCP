#include "dhcp_server.h"

int main() {
    // Configurar el manejo de señales
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    // Configurar el rango de IPs
    ip_range_t range;
    initialize_ip_pool(&range, "192.168.1.100", "192.168.1.200", 1);
    // Inicializar el servidor DHCP
    init_dhcp_server(&range);
    return 0;
}
