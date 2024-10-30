# Casos de Prueba para Proyecto DHCP

Este documento describe los casos de prueba implementados para verificar el correcto funcionamiento del sistema DHCP desarrollado. El script `dhcp_tests.sh` ejecuta estos casos de prueba de forma automática.

## Caso de Prueba 1: Asignación de IP válida

**Descripción:** Este caso verifica que el servidor DHCP asigne una dirección IP válida al cliente. Se ejecuta un cliente, y se espera un tiempo mínimo de 32 segundos para comprobar que el cliente renueva su IP correctamente y que la expiración del lease se realiza como se espera.

**Criterio de éxito:** El cliente obtiene una IP y se renueva correctamente sin errores.

## Caso de Prueba 2: Concurrencia de Clientes

**Descripción:** Este caso verifica la capacidad del sistema para manejar múltiples solicitudes concurrentes. Se ejecutan dos clientes simultáneamente para observar cómo el servidor y el relay manejan la concurrencia y asignan direcciones IP distintas a cada cliente.

**Criterio de éxito:** Ambos clientes obtienen direcciones IP únicas sin errores de concurrencia.

## Caso de Prueba 3: Agotamiento de IPs

**Descripción:** Este caso verifica el comportamiento del servidor DHCP cuando se agotan las direcciones IP disponibles. El servidor tiene configurado un límite de 10 IPs, y se lanzan 12 clientes para comprobar que, al superar el límite, el servidor informa que no hay más IPs disponibles.

**Criterio de éxito:** Los primeros 10 clientes obtienen direcciones IP válidas, y los clientes adicionales reciben un mensaje de error indicando la falta de IPs disponibles.

---

Cada caso de prueba se ejecuta con un intervalo de al menos 32 segundos para permitir la renovación y expiración de las direcciones IP asignadas.
