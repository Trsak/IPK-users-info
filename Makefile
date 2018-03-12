CC = g++ -static-libstdc++
CFLAGS = -c -Wall
LDFLAGS = -g

CLIENT_SOURCES = client.cpp
SERVER_SOURCES = server.cpp

CLIENT_OBJECTS = $(CLIENT_SOURCES:.cpp=.o)
SERVER_OBJECTS = $(SERVER_SOURCES:.cpp=.o)

CLIENT_EXECUTABLE = ipk-client
SERVER_EXECUTABLE = ipk-server

.PHONY: all client server clean remove

all: client server

client: $(CLIENT_EXECUTABLE)

server: $(SERVER_EXECUTABLE)

$(CLIENT_EXECUTABLE): $(CLIENT_OBJECTS)
	$(CC) $(LDFLAGS) $(CLIENT_OBJECTS) -o $@

$(SERVER_EXECUTABLE): $(SERVER_OBJECTS)
	$(CC) $(LDFLAGS) $(SERVER_OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f *.o

remove: clean
	rm -f $(CLIENT_EXECUTABLE) $(SERVER_EXECUTABLE)
