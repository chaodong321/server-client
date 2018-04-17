.PHONY: all clean

CC = gcc
CFLAGS = -g -Wall
#LIBS = -lpthread

#DEBUG = -D MYDEBUG_DD
DEBUG = 

SERVER = tcp-server
SERVER_SRCS = tcp-server.o

CLIENT = tcp-client
CLIENT_SRCS = tcp-client.o

all: $(SERVER) $(CLIENT)

$(SERVER):$(SERVER_SRCS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS) $(DEBUG)

$(CLIENT):$(CLIENT_SRCS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS) $(DEBUG)

%.o:%.c
	$(CC) $(CFLAGS) -c $< -o $@ $(DEBUG)

clean:
	rm $(SERVER) $(SERVER_SRCS) $(CLIENT) $(CLIENT_SRCS)