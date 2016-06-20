CC=g++
CFLAGS=-c -Wall -std=c++11
LDFLAGS=-lpthread
SOURCES=args.cc message.cc rpc_client.cc rpc_server.cc
OBJECTS=$(SOURCES:.cc=.o)
LIBRARY=librpc.a
LC=ar rcs

all: $(SOURCES) $(LIBRARY)

$(LIBRARY): $(OBJECTS)
	$(LC) $(LIBRARY) $(OBJECTS)

$(CLIENT): $(CLIENT_OBJS)

.cc.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm $(LIBRARY) $(OBJECTS)
