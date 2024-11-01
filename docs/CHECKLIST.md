# Lista de requisitos para el proyecto de DHCP

## Aplicación Cliente
- [x] Puede ser programada en cualquier lenguaje (Python, Java, C, etc.).
- [x] Debe enviar una solicitud de dirección IP (mensaje DHCPDISCOVER) al servidor.
- [x] Recibir una dirección IP y mostrar la información proporcionada por el servidor (dirección IP, máscara de red, puerta de enlace predeterminada, servidor DNS, etc.).
- [x] Capacidad de solicitar la renovación de la dirección IP cuando sea necesario.
- [x] Liberar la dirección IP al terminar la ejecución.

## Aplicación Servidor
- [x] Debe ser implementada en lenguaje C.
- [ ] Escuchar solicitudes DHCP de clientes en una red local o remota.
- [x] Asignar direcciones IP disponibles de manera dinámica a los clientes que lo soliciten.
- [x] Gestionar un rango de direcciones IP definido por el usuario para asignar a los clientes.
- [x] Gestionar correctamente la concesión (lease) de las direcciones IP, incluyendo renovación y liberación.
- [x] Manejar múltiples solicitudes simultáneamente (concurrencia).
- [x] Registrar todas las asignaciones de direcciones IP y el tiempo de arrendamiento.
- [x] Soportar las fases y mensajes principales del proceso DHCP: DISCOVER, OFFER, REQUEST, ACK (opcionalmente NAK, DECLINE, y RELEASE).
- [x] Atender solicitudes de clientes en subredes diferentes mediante el uso de un DHCP relay.
- [x] Manejar correctamente errores y condiciones especiales (por ejemplo, falta de direcciones IP disponibles).
- [x] Soportar concurrencia utilizando hilos para gestionar múltiples peticiones simultáneas.

## Proyecto en general
- [x] La aplicación cliente puede ser desarrollada en cualquier lenguaje que soporte la API de Berkeley Sockets.
- [x] La aplicación servidor debe ser implementada exclusivamente en lenguaje C.
- [x] No utilizar clases de sockets existentes o personalizadas, solo la API de Sockets Berkeley.
- [ ] Desplegar el servidor en un servidor en la nube utilizando AWS Academy.
- [x] Documentar el diseño e implementación usando herramientas como UML.
- [x] Trabajar en equipos de hasta 3 personas.
- [x] Definir un cronograma con hitos y victorias tempranas, dedicando el 60% del tiempo al servidor y 40% al cliente.
- [x] Usar un repositorio Git privado para la gestión del código.
- [x] Documentar el proyecto en un archivo README.md con las secciones: Introducción, Desarrollo, Aspectos Logrados/No logrados, Conclusiones y Referencias.
- [ ] Crear un video explicando el proceso de diseño, desarrollo y ejecución, no mayor a 15 minutos.
- [x] Entregar el proyecto antes del 18 de octubre de 2024 mediante el buzón de Interactiva Virtual, incluyendo el enlace al repositorio GitHub.