CC=gcc

UV_PATH=$(shell pwd)/libuv-1.37.0
UV_LIBS_PATH=$(UV_PATH)/.libs
UV_LIB=$(UV_LIBS_PATH)/libuv.a

CFLAGS=-Wall -O0 -g -std=c11 -D_XOPEN_SOURCE=600 -I/usr/include -I$(UV_PATH)/include
LDFLAGS=-L/usr/lib -lc -ldl -lpthread -lm -lrt
SOURCES=main.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=pynogil

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) $(UV_LIB) -o $@

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm $(OBJECTS)
	rm $(EXECUTABLE)
