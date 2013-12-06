# this is makefile of torus

VPATH=.
DEPS=./deps/spatialindex
SOCKETDIR=./socket
TORUSDIR=./torus_node
CONTROLDIR=./control_node
SKIPLISTDIR=./skip_list
LOGDIR=./logs
TESTDIR=./test
BIN=./bin
CC=gcc
CFLAGS=-g -lrt -Wall -lstdc++

all: control start-node test1

control: control.o torus-node.o socket.o skip-list.o log.o
	$(CC) $(CFLAGS) -o $(BIN)/$@ $(BIN)/control.o $(BIN)/torus-node.o $(BIN)/socket.o $(BIN)/skip-list.o $(BIN)/log.o

start-node:
	$(CC)  $(BIN)/torus-node.o $(BIN)/server.o $(BIN)/socket.o $(BIN)/skip-list.o $(BIN)/log.o $(BIN)/start-node.o -o $(BIN)/$@ $(CFLAGS) -I$(DEPS)/include -I$(VPATH) -L$(DEPS)/lib -lspatialindex 

control.o:
	$(CC) $(CFLAGS) -o $(BIN)/$@ -c $(CONTROLDIR)/control.c -I$(VPATH)

torus-node.o:
	$(CC) $(CFLAGS) -o $(BIN)/$@ -c $(TORUSDIR)/torus_node.c -I$(VPATH)
	
socket.o:
	$(CC) $(CFLAGS) -o $(BIN)/$@ -c $(SOCKETDIR)/socket.c -I$(VPATH)

skip-list.o:
	$(CC) $(CFLAGS) -o $(BIN)/$@ -c $(SKIPLISTDIR)/skip_list.c -I$(VPATH)
	
start-node.o: torus_node/torus_node.h torus_node/server.h socket/socket.h skip_list/skip_list.h logs/log.h
	$(CC)  -o $(BIN)/$@ -c $(TORUSDIR)/start_node.cc -I$(VPATH) $(CFLAGS) -I$(DEPS)/include -L$(DEPS)/lib -lspatialindex

server.o:
	$(CC) $(CFLAGS) -o $(BIN)/$@ -c $(TORUSDIR)/server.c -I$(VPATH) -lrt
	
log.o:
	$(CC) $(CFLAGS) -o $(BIN)/$@ -c $(LOGDIR)/log.c -I$(VPATH)

test1:
	$(CC) -g -o $(BIN)/$@ test.c
	
.PHONY: clean
clean:
	-rm $(BIN)/*.o
	-rm $(BIN)/control $(BIN)/start-node $(BIN)/test1




