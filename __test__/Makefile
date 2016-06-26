CC=g++
CFLAGS=-c -Wall -std=c++11
LDFLAGS=-lpthread
SOURCES=$(wildcard *.cc)
OBJECTS=$(SOURCES:.cc=.o)
EXECUTABLE=test

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

.cc.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm $(EXECUTABLE) $(OBJECTS)
