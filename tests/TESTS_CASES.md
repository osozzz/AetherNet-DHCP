# Casos de prueba para DHCP

## Caso 1: Solicitud de IP básica
Descripción: El cliente envía un mensaje DHCP Discover, y el servidor responde con un DHCP Offer. El cliente acepta con un DHCP Request, y el servidor confirma con un DHCP ACK.
Resultado esperado: El cliente debe recibir una dirección IP asignada.

## Caso 2: Múltiples clientes
Descripción: Varios clientes (al menos 5) envían mensajes DHCP Discover simultáneamente, y el servidor les asigna IPs únicas.
Resultado esperado: Cada cliente debe recibir una IP única.

## Caso 3: Agotamiento de direcciones IP
Descripción: El servidor DHCP asigna todas las direcciones IP disponibles. Los clientes que soliciten después de que se agoten las IP deben recibir un mensaje de error.
Resultado esperado: El cliente debe recibir un mensaje que indique que no hay IPs disponibles.

## Caso 4: Expiración de lease
Descripción: El lease de una dirección IP expira y el cliente envía una solicitud de renovación. El servidor debe responder con la misma IP si está disponible o con una nueva IP si la anterior ha sido reasignada.
Resultado esperado: El cliente debe recibir la misma IP si está disponible, o una nueva si no.

## Caso 5: Cliente libera la IP
Descripción: El cliente envía un mensaje DHCP Release para liberar la dirección IP antes de que expire el lease.
Resultado esperado: El servidor debe liberar la IP y esta debe estar disponible para otros clientes.

## Caso 6: Solicitud de renovación de lease
Descripción: El cliente solicita la renovación de su lease antes de que expire, usando un mensaje DHCP Request.
Resultado esperado: El servidor debe renovar el lease con la misma dirección IP si sigue disponible.

## Caso 7: Cliente malformado (paquete DHCP incorrecto)
Descripción: El cliente envía un paquete DHCP malformado al servidor, por ejemplo, con un formato incorrecto o con datos faltantes.
Resultado esperado: El servidor debe rechazar el paquete y registrar un error sin afectar la estabilidad del sistema.

## Caso 8: Tamaño del paquete supera el tamaño de buffer
Descripción: El cliente envía un paquete DHCP que excede el tamaño del buffer esperado por el servidor.
Resultado esperado: El servidor debe rechazar el paquete e informar un error de tamaño de paquete.

## Caso 9: IP en blacklist
Descripción: El servidor tiene una lista de IPs en blacklist (por ejemplo, IPs conflictivas o reservadas), y un cliente solicita una IP de esa lista.
Resultado esperado: El servidor debe rechazar la solicitud y asignar una IP fuera de la lista negra.
