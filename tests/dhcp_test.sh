#!/bin/bash
# Script de prueba para el cliente y servidor DHCP

# Iniciar el servidor DHCP
echo "Iniciando el servidor DHCP..."
../src/server/dhcp_server & disown # Inicia el servidor en segundo plano
SERVER_PID=$!

# Dar tiempo al servidor para inicializar
sleep 2

# Ejecutar casos de prueba
echo "Ejecutando pruebas..."

# 1. Prueba de solicitud básica de IP
echo "Prueba 1: Cliente solicita una IP..."
../src/client/dhcp_client
echo "Cliente debería recibir una IP asignada."

echo "============================================="
# 2. Prueba de múltiples clientes solicitando IPs
echo "Prueba 2: Múltiples clientes solicitando IPs..."
for i in {1..5}
do
    echo "Cliente $i solicitando IP..."
    ../src/client/dhcp_client &
done
wait
echo "Todos los clientes deberían recibir IPs."

kill $SERVER_PID
../src/server/dhcp_server & disown
SERVER_PID=$!

echo "============================================="
# 3. Prueba de agotamiento de direcciones IP
echo "Prueba 3: Agotamiento de direcciones IP..."
for i in {1..55}  # Asumiendo que el servidor tiene un límite de 50 IPs
do
    echo "Cliente $i solicitando IP..."
    ../src/client/dhcp_client &
done
wait
echo "Se debería ver un mensaje de error cuando no haya más IPs disponibles."

kill $SERVER_PID
../src/server/dhcp_server & disown
SERVER_PID=$!

echo "============================================="
# Detener el servidor DHCP
echo "Deteniendo el servidor DHCP..."
kill $SERVER_PID
