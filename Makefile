CC=g++
CFLAGS=-c -Wall -std=c++11
LDFLAGS=-lpthread
SOURCES=args.cc binder.cc message.cc rpc_client.cc rpc_server.cc
EXEC_OBJECTS=binder.o
LIB_OBJECTS=rpc_client.o rpc_server.o
SHARED_OBJECTS=args.o message.o
OBJECTS=$(LIB_OBJECTS) $(EXEC_OBJECTS) $(SHARED_OBJECTS)
LIBRARY=librpc.a
EXECUTABLE=binder
LC=ar rcs

all: $(SOURCES) $(LIBRARY) $(EXECUTABLE)

$(LIBRARY): $(LIB_OBJECTS) $(SHARED_OBJECTS)
	$(LC) $(LIBRARY) $(LIB_OBJECTS) $(SHARED_OBJECTS)

$(EXECUTABLE): $(EXEC_OBJECTS) $(SHARED_OBJECTS)
	$(CC) $(EXEC_OBJECTS) $(SHARED_OBJECTS) -o $@ $(LDFLAGS)

.cc.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm $(LIBRARY) $(EXECUTABLE) $(OBJECTS)
