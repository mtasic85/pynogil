CC=gcc

UV_PATH=$(shell pwd)/libuv
UV_LIBS_PATH=$(UV_PATH)/.libs
UV_LIB=$(UV_LIBS_PATH)/libuv.a

DUKTAPE_PATH=$(shell pwd)/duktape/dist

CFLAGS=-Wall -O0 -g -std=c11 -D_XOPEN_SOURCE=600 -I$(UV_PATH)/include -I$(DUKTAPE_PATH)/src -I$(DUKTAPE_PATH)/extras/console -I$(DUKTAPE_PATH)/extras/console -I$(DUKTAPE_PATH)/extras/module-node
LDFLAGS=-L/usr/lib -lc -ldl -lpthread -lm -lrt
SOURCES=$(DUKTAPE_PATH)/src/duktape.c $(DUKTAPE_PATH)/extras/console/duk_console.c $(DUKTAPE_PATH)/extras/module-node/duk_module_node.c duk_poll.c pynogil.c main.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=pynogil

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) $(UV_LIB) -o $@

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm $(EXECUTABLE)
	rm $(OBJECTS)
