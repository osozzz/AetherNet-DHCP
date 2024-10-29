CC = gcc
CFLAGS = -Wall -g

CLIENT_OBJS = dhcp_relay.o
MAIN_OBJS = main.o
TARGET = dhcp_relay

all: $(TARGET)

$(TARGET): $(CLIENT_OBJS) $(MAIN_OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(CLIENT_OBJS) $(MAIN_OBJS)

dhcp_client.o: dhcp_relay.c dhcp_relay.h
	$(CC) $(CFLAGS) -c dhcp_relay.c -o dhcp_relay.o

main.o: main.c dhcp_relay.h
	$(CC) $(CFLAGS) -c main.c -o main.o

clean:
	rm -f $(TARGET) $(CLIENT_OBJS) $(MAIN_OBJS)
