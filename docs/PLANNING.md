# 📅 **Plan de Desarrollo del Proyecto - Servidor DHCP**

Este plan detalla las fases, hitos, y tareas del proyecto de implementación de un **Servidor DHCP** en lenguaje C, utilizando la API de **Sockets Berkeley** y respetando los requisitos establecidos.

## 🛠️ **Fase 1: Configuración Inicial del Servidor DHCP**

### 🎯 Hitos:

- 📍 **Iniciar el servidor DHCP** utilizando **sockets UDP** (SOCK_DGRAM) para escuchar en el puerto 67.
- 📍 **Escuchar solicitudes DHCP Discover** de clientes locales o remotos.
- 📍 **Enviar respuestas DHCP Offer** a los clientes.

### 📝 Tareas:

- ✅ Configurar los sockets con **SOCK_DGRAM** (sin conexión) para escuchar en el puerto 67 (UDP).
- ✅ Implementar la recepción de solicitudes **DHCP Discover**.
- ✅ Implementar el envío de respuestas **DHCP Offer** utilizando la API de Berkeley.
- ✅ Justificar el uso de **UDP (SOCK_DGRAM)** con base en los requerimientos de la aplicación.
  
### 🏆 **Victoria Temprana:**

El servidor puede recibir una solicitud DHCP Discover y responder con un DHCP Offer.

---

## 🛠️ **Fase 2: Gestión de Direcciones IP (Asignación y Renovación)**

### 🎯 Hitos:

- 📍 **Asignar dinámicamente direcciones IP** a los clientes desde un rango definido por el usuario.
- 📍 **Gestionar concesiones (lease)** de direcciones IP, incluyendo el tiempo de arrendamiento y la renovación.
- 📍 **Soportar fases Request y ACK** para completar el proceso DHCP.

### 📝 Tareas:

- ✅ Definir un rango de direcciones IP dinámicas que el servidor puede asignar.
- ✅ Implementar la concesión (lease) de direcciones IP, almacenando las asignaciones y tiempos.
- ✅ Soportar las fases **DHCP Request** (cliente solicita dirección) y **DHCP ACK** (servidor confirma).
- ✅ Manejar **NAK**, **Decline** y **Release** en situaciones de error o liberación de IP.

### 🏆 **Victoria Temprana:**

El servidor puede asignar una dirección IP válida a un cliente, con un tiempo de concesión gestionado.

---

## 🛠️ **Fase 3: Concurrencia y Manejo de Múltiples Solicitudes**

### 🎯 Hitos:

- 📍 **Implementar concurrencia** utilizando hilos (`pthread`) para gestionar múltiples solicitudes simultáneas.
- 📍 **Manejo eficiente de solicitudes concurrentes**, evitando conflictos en la asignación de direcciones IP.

### 📝 Tareas:

- ✅ Implementar el uso de **hilos (pthreads)** para manejar múltiples solicitudes de clientes al mismo tiempo.
- ✅ Gestionar correctamente las solicitudes simultáneas, garantizando que las direcciones IP no se dupliquen ni causen conflictos.
- ✅ Probar el servidor con varios clientes a la vez.

### 🏆 **Victoria Temprana:**

El servidor puede manejar múltiples clientes simultáneamente y asignar direcciones IP de manera correcta y sin conflictos.

---

## 🛠️ **Fase 4: Soporte para Clientes en Subredes Diferentes (DHCP Relay)**

### 🎯 Hitos:

- 📍 **Soportar clientes en subredes diferentes** utilizando un **DHCP Relay** que reenvíe las solicitudes al servidor.
- 📍 **Asegurar la correcta respuesta a clientes remotos** a través del relay.

### 📝 Tareas:

- ✅ Implementar la lógica para manejar solicitudes DHCP a través de un **DHCP Relay**.
- ✅ Configurar y probar un entorno con un cliente en una subred remota y un relay DHCP que reenvíe las solicitudes.
- ✅ Verificar que el servidor responda correctamente a los clientes en subredes remotas.

### 🏆 **Victoria Temprana:**

El servidor DHCP puede gestionar solicitudes y asignar direcciones a clientes en subredes diferentes, utilizando un relay DHCP.

---

## 🛠️ **Fase 5: Manejo de Errores y Condiciones Especiales**

### 🎯 Hitos:

- 📍 **Manejar situaciones especiales**, como la falta de direcciones IP disponibles o solicitudes mal formadas.
- 📍 **Implementar respuestas apropiadas** (por ejemplo, NAK) cuando las solicitudes no pueden ser procesadas correctamente.

### 📝 Tareas:

- ✅ Implementar la lógica para manejar errores, como cuando se agotan las direcciones IP.
- ✅ Enviar mensajes **DHCP NAK** cuando no se puede asignar una dirección.
- ✅ Probar las respuestas a solicitudes mal formadas o no válidas.

### 🏆 **Victoria Temprana:**

El servidor maneja correctamente los errores y condiciones especiales, proporcionando mensajes de error claros (como DHCP NAK).

---

## 🛠️ **Fase 6: Pruebas Exhaustivas y Validación**

### 🎯 Hitos:

- 📍 **Pruebas exhaustivas de todas las fases DHCP**: Discover, Offer, Request, ACK, NAK, Release, con múltiples clientes.
- 📍 **Captura y análisis del tráfico DHCP** utilizando **Wireshark** para asegurar que los mensajes cumplen con el estándar.

### 📝 Tareas:

- ✅ Realizar pruebas unitarias para cada fase del ciclo de vida DHCP.
- ✅ Probar la funcionalidad con múltiples clientes simultáneos y en diferentes subredes.
- ✅ Capturar y analizar el tráfico DHCP con **Wireshark** para validar los mensajes.
  
### 🏆 **Victoria Temprana:**

El servidor DHCP pasa todas las pruebas de funcionalidad y concurrencia, con tráfico validado mediante captura en Wireshark.

---

## 🛠️ **Fase 7: Despliegue en AWS y Documentación Final**

### 🎯 Hitos:

- 📍 **Desplegar el servidor en AWS**, usando una instancia EC2 que soporte la funcionalidad completa del servidor DHCP.
- 📍 **Completar la documentación** del proyecto, incluyendo el archivo `README.md`, diagramas UML, y referencias.

### 📝 Tareas:

- ✅ Configurar y desplegar una **instancia EC2 en AWS** con el servidor DHCP.
- ✅ Probar la accesibilidad y funcionalidad del servidor desde la instancia EC2.
- ✅ Completar la documentación final del proyecto, incluyendo:
  - Diagrama UML del diseño del sistema.
  - Manual de usuario para desplegar y usar el servidor.
  - Informe de los aspectos logrados y no logrados.

### 🏆 **Victoria Final:**

El servidor DHCP está desplegado y funcionando en AWS. La documentación está completa y subida al repositorio, cumpliendo con todos los requisitos.

---

## 📈 **Cronograma Estimado**

| Fase                                 | Duración Estimada |
|--------------------------------------|------------------|
| 🛠️ Fase 1: Configuración Básica       | 1 semana         |
| 🛠️ Fase 2: Gestión de IPs             | 1.5 semanas      |
| 🛠️ Fase 3: Concurrencia               | 1 semana         |
| 🛠️ Fase 4: DHCP Relay                 | 1 semana         |
| 🛠️ Fase 5: Manejo de Errores          | 0.5 semana       |
| 🛠️ Fase 6: Pruebas y Validación       | 1 semana         |
| 🛠️ Fase 7: Despliegue y Documentación | 1 semana         |

---
