# Definir el compilador
CC = gcc
CFLAGS = -Wall -g

# Archivos fuente y ejecutable
SOURCES = dhcp_relay.c main.c
TARGET = dhcp_relay

# Regla por defecto
all: $(TARGET)

# Compilación del ejecutable y eliminación de objetos
$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES)
	@echo "Eliminando archivos objeto..."
	@rm -f *.o

# Limpiar archivos objeto y ejecutable
clean:
	rm -f *.o $(TARGET)
