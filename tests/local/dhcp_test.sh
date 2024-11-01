#!/bin/bash

# Directorios de los binarios
CLIENT_BIN="./dhcp_client"
RELAY_BIN="./dhcp_relay"
SERVER_BIN="./dhcp_server"

# Iniciar el servidor y el relay en el fondo
start_server_and_relay() {
    echo "Iniciando el servidor DHCP..."
    $SERVER_BIN &
    SERVER_PID=$!
    
    echo "Iniciando el relay DHCP..."
    $RELAY_BIN &
    RELAY_PID=$!
    
    # Espera un momento para asegurar que el servidor y relay están listos
    sleep 5
}

# Detener el servidor y relay
stop_server_and_relay() {
    echo "Deteniendo el servidor y el relay..."
    kill $SERVER_PID $RELAY_PID
    wait $SERVER_PID $RELAY_PID 2>/dev/null
}

# Caso de prueba 1: Asignación de IP válida
test_ip_assignment() {
    echo "Caso de prueba 1: Asignación de IP válida"

    start_server_and_relay

    echo "Iniciando cliente DHCP..."
    $CLIENT_BIN

    echo "Esperando 32 segundos para validar la renovación y expiración..."
    sleep 35

    stop_server_and_relay
    echo "Prueba de asignación de IP válida completada."
}

# Caso de prueba 2: Concurrencia de clientes
test_concurrent_clients() {
    echo "Caso de prueba 2: Concurrencia de clientes"

    start_server_and_relay

    echo "Iniciando dos clientes DHCP simultáneamente..."
    $CLIENT_BIN &
    CLIENT1_PID=$!
    $CLIENT_BIN &
    CLIENT2_PID=$!

    echo "Esperando 32 segundos para observar el funcionamiento concurrente..."
    sleep 40

    echo "Deteniendo los clientes..."
    kill $CLIENT1_PID $CLIENT2_PID
    wait $CLIENT1_PID $CLIENT2_PID 2>/dev/null

    stop_server_and_relay
    echo "Prueba de concurrencia completada."
}

# Caso de prueba 3: Agotamiento de IPs
test_ip_exhaustion() {
    echo "Caso de prueba 3: Agotamiento de IPs"

    start_server_and_relay

    echo "Iniciando 12 clientes DHCP para verificar el agotamiento de IPs..."
    for i in {1..4}; do
        $CLIENT_BIN &
        CLIENT_PIDS[$i]=$!
    done

    echo "Esperando 32 segundos para verificar el agotamiento..."
    sleep 300

    echo "Deteniendo todos los clientes..."
    for pid in "${CLIENT_PIDS[@]}"; do
        kill $pid
    done
    wait "${CLIENT_PIDS[@]}" 2>/dev/null

    stop_server_and_relay
    echo "Prueba de agotamiento de IPs completada."
}

# Ejecución de las pruebas
test_ip_assignment
echo
test_concurrent_clients
echo
test_ip_exhaustion
