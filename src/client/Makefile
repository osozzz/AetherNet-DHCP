CC = gcc
CFLAGS = -Wall -g

CLIENT_OBJS = dhcp_client.o
MAIN_OBJS = main.o
TARGET = dhcp_client

all: $(TARGET)

$(TARGET): $(CLIENT_OBJS) $(MAIN_OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(CLIENT_OBJS) $(MAIN_OBJS)

dhcp_client.o: dhcp_client.c dhcp_client.h
	$(CC) $(CFLAGS) -c dhcp_client.c -o dhcp_client.o

main.o: main.c dhcp_client.h
	$(CC) $(CFLAGS) -c main.c -o main.o

clean:
	rm -f $(TARGET) $(CLIENT_OBJS) $(MAIN_OBJS)
