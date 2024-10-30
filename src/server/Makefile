# Definir el compilador
CC = gcc

# Opciones de compilación
CFLAGS = -Wall -g

# Archivos fuente
SOURCES = dhcp_server.c main.c

# Nombre del ejecutable
TARGET = dhcp_server

# Regla por defecto
all: $(TARGET)

# Regla para construir el ejecutable y eliminar los objetos
$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES)
	@echo "Eliminando archivos objeto..."
	@rm -f *.o

# Limpiar los archivos objeto y el ejecutable
clean:
	rm -f *.o $(TARGET)
