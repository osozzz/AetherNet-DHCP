## **🚀 Instalación**

### **Requisitos Previos** 🛠️

Antes de comenzar con la instalación, asegúrate de tener lo siguiente:

- **Sistema Operativo**: Ubuntu 24.04.1 LTS, MSYS2 (27 julio 2024), y WSL2 2.0.
- **Compilador GCC**: Para compilar los archivos en C.
- **Make**: Se usa un archivo Makefile para facilitar la compilación.
- **Permisos de Root**: Algunos comandos como la asignación de IPs pueden necesitar permisos de administrador.

### **1. Clonar el Repositorio** 📥

Primero, clona el repositorio del proyecto desde GitHub:

```bash
git clone https://github.com/osozzz/AetherNet-DHCP.wiki.git
```

### **2. Compilar el Proyecto** ⚙️

Cada componente del proyecto tiene su propio directorio. A continuación se muestran los pasos para compilar cada uno:

#### **💻 Compilar el Servidor DHCP**

1. Entra en el directorio del servidor con el siguiente comando:

   ```bash
   cd src/server
   ```
2. Ejecuta el siguiente comando para limpiar cualquier archivo de compilación previo y luego compilar el servidor:

   ```bash
   make clean && make
   ```

3. Una vez compilado, puedes ejecutar el servidor con permisos de superusuario:

   ```bash
   sudo ./dhcp_server
   ```

#### **🖥️ Compilar el Cliente DHCP**

1. Entra en el directorio del cliente con el siguiente comando:

   ```bash
   cd src/client
   ```
2. Ejecuta el siguiente comando para limpiar cualquier archivo de compilación previo y luego compilar el cliente:

   ```bash
   make clean && make
   ```

3. Una vez compilado, puedes ejecutar el cliente con permisos de superusuario:

   ```bash
   sudo ./dhcp_client
   ```

#### **🔄 Compilar el Relay DHCP**

1. Entra en el directorio del relay con el siguiente comando:

   ```bash
   cd src/relay
   ```
2. Ejecuta el siguiente comando para limpiar cualquier archivo de compilación previo y luego compilar el relay:

   ```bash
   make clean && make
   ```

3. Una vez compilado, puedes ejecutar el relay con permisos de superusuario:

   ```bash
   sudo ./dhcp_relay
   ```

#### **🧪 Compilar los Tests**

1. Entra en el directorio de los tests con el siguiente comando:

   ```bash
   cd src/tests
   ```
2. Ejecuta el siguiente comando para limpiar cualquier archivo de compilación previo y luego compilar el test:

   ```bash
   make clean && make
   ```

3. Una vez compilado, puedes ejecutar el test con permisos de superusuario:

   ```bash
   sudo ./dhcp_tests
   ```

## **💡 Consideraciones Adicionales**

- **⚠️ Permisos de Superusuario:** Para ejecutar algunos componentes, como el servidor y el relay, es posible que necesites permisos de superusuario (`sudo`), ya que estos componentes requieren acceso a puertos restringidos (por debajo de 1024).

- **🌐 Entorno de Red:** Asegúrate de que la red en la que estés probando tenga las configuraciones adecuadas para evitar interferencias con otros servidores DHCP en la misma red.

## **🏁 Conclusión**

Siguiendo estos pasos, deberías poder compilar y ejecutar con éxito tanto el servidor como el cliente DHCP, junto con el relay y los tests.
