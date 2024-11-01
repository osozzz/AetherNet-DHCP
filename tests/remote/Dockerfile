# Dockerfile
FROM ubuntu:latest

# Establecer el directorio de trabajo
WORKDIR /app

# Instalar dependencias
RUN apt-get update && apt-get install -y \
    iputils-ping \
    iproute2 \
    net-tools \
    build-essential \
    && rm -rf /var/lib/apt/lists/*

# Copiar todos los binarios y el script wait-for-it.sh desde `tests/remote`
COPY tests/remote/dhcp_server /app/server
COPY tests/remote/dhcp_relay /app/relay
COPY tests/remote/dhcp_client /app/client

# Permitir ejecución de cada aplicación
RUN chmod +x /app/server /app/relay /app/client

# Exponer los puertos necesarios
EXPOSE 67/udp
EXPOSE 68/udp

# Comando a ejecutar, configuraremos esto al momento de correr cada contenedor
CMD ["/app/server"]
