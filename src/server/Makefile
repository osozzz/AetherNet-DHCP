# Definir el compilador
CC = gcc

# Opciones de compilación
CFLAGS = -Wall -g

# Archivos fuente
SOURCES = dhcp_server.c main.c

# Archivos objeto
OBJECTS = $(SOURCES:.c=.o)

# Nombre del ejecutable
TARGET = dhcp_server

# Regla por defecto
all: $(TARGET)

# Regla para construir el ejecutable
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS)

# Limpiar los archivos objeto y el ejecutable
clean:
	rm -f $(OBJECTS) $(TARGET)