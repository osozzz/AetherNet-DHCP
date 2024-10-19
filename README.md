# Readme del Proyecto AetherNet DHCP

¡Bienvenido al Proyecto AetherNet DHCP! Este Readme sirve como una guía integral para entender el diseño, desarrollo y funcionalidad del servidor y cliente DHCP de AetherNet. A continuación, se presentan las diferentes secciones de la documentación del proyecto.

---

## 📜 **Introducción**

AetherNet DHCP es un proyecto que tiene como objetivo implementar un servidor y cliente DHCP (Dynamic Host Configuration Protocol) totalmente funcional. El servidor es capaz de asignar direcciones IP de manera dinámica a los clientes que las solicitan, gestionar los arrendamientos (leases) de IPs, manejar múltiples solicitudes simultáneamente y asignar direcciones IP eficientemente para evitar conflictos.

El cliente interactúa con el servidor para solicitar direcciones IP, gestionar la renovación de los arrendamientos y detectar conflictos de direcciones. Este proyecto está desarrollado en C, utilizando la API de Berkeley Sockets, y está diseñado para ejecutarse en entornos de nube (como AWS) para simular escenarios de despliegue en el mundo real.

---

## 🔧 **Desarrollo**

### **Servidor DHCP**
El servidor se encarga de realizar las siguientes tareas:
- **Gestión de mensajes DHCP (Discover, Offer, Request, Acknowledgment):** El servidor escucha las solicitudes de los clientes y responde con los mensajes adecuados del protocolo DHCP.
- **Multithreading:** El servidor utiliza hilos para manejar múltiples solicitudes de manera concurrente, mejorando el rendimiento y la escalabilidad.
- **Gestión de arrendamientos:** El servidor asigna direcciones IP con una duración específica y se asegura de que las IPs se renueven o liberen correctamente cuando expiran.
- **Detección de conflictos:** Los clientes pueden notificar al servidor sobre conflictos de IP mediante mensajes DHCPDECLINE, permitiendo que el servidor marque esas direcciones IP como no disponibles.

### **Cliente DHCP**
El cliente realiza las siguientes funciones:
- **Solicitud de direcciones IP:** El cliente envía un mensaje DHCPDISCOVER y maneja la respuesta DHCPOFFER del servidor.
- **Renovación de arrendamiento:** El cliente gestiona el arrendamiento de su dirección IP, renovándolo antes de que expire.
- **Detección de conflictos:** El cliente detecta conflictos de IP utilizando ARP ping y envía mensajes DHCPDECLINE si se detectan conflictos.

---

## ✅ **Aspectos Logrados y No logrados**

### **Aspectos Logrados:**
1. **Implementar la funcionalidad básica del servidor DHCP:** Se configuró el servidor para manejar los mensajes DHCPDISCOVER, DHCPOFFER, DHCPREQUEST y DHCPACK.
2. **Implementar la funcionalidad básica del cliente DHCP:** Se desarrolló un cliente capaz de enviar mensajes DHCPDISCOVER y manejar las respuestas DHCPOFFER, DHCPREQUEST y DHCPACK.
3. **Multithreading para solicitudes concurrentes de clientes:** El servidor fue implementado con multithreading para gestionar múltiples solicitudes simultáneamente.
4. **Estrategia de asignación de direcciones IP:** Se desarrolló lógica para evitar asignaciones duplicadas y reutilizar direcciones IP después de la expiración del arrendamiento.
5. **Gestión del tiempo de arrendamiento:** Se implementó la gestión de arrendamientos, incluyendo los temporizadores T1 y T2 para la renovación.
6. **Soporte para DHCPDECLINE y detección de conflictos de IP:** Se agregó el manejo de mensajes DHCPDECLINE cuando los clientes detectan conflictos de IP.
7. **Soporte para mensajes DHCPRELEASE:** Se añadió la capacidad de que los clientes liberen sus direcciones IP cuando ya no las necesiten.
8. **Detección de conflictos de IP en el cliente:** El cliente puede detectar conflictos de IP utilizando ARP ping y enviar mensajes DHCPDECLINE.
9. **Manejo de DHCPNAK:** Se implementó la gestión de mensajes DHCPNAK en el cliente, activando una nueva solicitud de IP.
10. **Pruebas unitarias para servidor y cliente:** Se desarrollaron pruebas unitarias básicas para garantizar la fiabilidad del servidor y del cliente.
11. **Apagado y limpieza del servidor de manera controlada:** Se implementó lógica para apagar el servidor de manera controlada y liberar los recursos como hilos y sockets.

### **Aspectos No logrados:**
- **Optimización avanzada de recursos:** Aunque se implementó la funcionalidad básica de multithreading, no se optimizó para la carga extrema de clientes concurrentes.
- **Escalabilidad en ambientes distribuidos:** No se incluyeron funciones avanzadas de escalabilidad para ejecutar el servidor en entornos distribuidos o con balanceo de carga.

---

## 📚 **Conclusiones**

El proyecto AetherNet DHCP demuestra la implementación de un servidor DHCP capaz de gestionar múltiples clientes de manera concurrente, asignando direcciones IP eficientemente y manejando los tiempos de arrendamiento. La funcionalidad de detección de conflictos asegura que no se asignen direcciones duplicadas, mejorando la robustez del sistema. Aunque el proyecto cumple con los requisitos básicos, hay margen para mejoras en términos de optimización y escalabilidad.

---

## 🔗 **Referencias**

- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/)
- [IETF RFC 2131 - Dynamic Host Configuration Protocol](https://datatracker.ietf.org/doc/html/rfc2131)
- [GeeksForGeeks: TCP Server-Client Implementation in C](https://www.geeksforgeeks.org/tcp-server-client-implementation-in-c/)
