ME=mfielbig
CC=gcc
UNP_DIR=/home/courses/cse533/Stevens/unpv13e
LIBS=/home/courses/cse533/Stevens/unpv13e/libunp.a
FLAGS=-g -O2 -Wall -Werror 
CFLAGS=$(FLAGS) -pthread -I$(UNP_DIR)/lib
EXE=odr_$(ME) server_$(ME) client_$(ME) odr_1_$(ME)
INCLUDES=common.h me.h uds.h

all: $(EXE)

odr_1_$(ME):uds.o odr_1.o odrether.o  pfsocket.o get_hw_addrs.o table.o
	gcc $(CFLAGS) -o $@ $^ ${LIBS}

odr_test_$(ME):uds.o odr_test.o pfsocket.o get_hw_addrs.o table.o
	gcc $(CFLAGS) -o $@ $^ ${LIBS}

odr_$(ME): uds.o odr.o pfsocket.o get_hw_addrs.o table.o queue.o odrether.o
	gcc $(CFLAGS) -o $@ $^ $(LIBS)

server_$(ME): uds.o server.o pfsocket.o get_hw_addrs.o
	gcc $(CFLAGS) -o $@ $^ $(LIBS)

client_$(ME): uds.o client.o
	gcc $(CFLAGS) -o $@ $^ $(LIBS)

table_test: table_test.o table.o
	gcc $(CFLAGS) -o $@ $^ $(LIBS)

queue_test: queue_test.o queue.o uds.o
	gcc $(CFLAGS) -o $@ $^ $(LIBS)

%.o: %.c %.h $(INCLUDES)
	gcc $(CFLAGS) -c $^

clean:
	rm -f *.o *.out *.gch $(EXE)
