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
CXX=g++
CFLAGS=-g -lrt -Wall

all: control start-node test1

control: control.o torus-node.o socket.o skip-list.o log.o
	$(CC) $(CFLAGS) -o $(BIN)/$@ $(BIN)/control.o $(BIN)/torus-node.o $(BIN)/socket.o $(BIN)/skip-list.o $(BIN)/log.o 

start-node: torus-node.o server.o socket.o skip-list.o log.o torus_rtree.o
	$(CXX) $(CFLAGS) $(BIN)/torus-node.o $(BIN)/server.o $(BIN)/socket.o $(BIN)/skip-list.o $(BIN)/log.o $(BIN)/torus_rtree.o -o $(BIN)/$@ -I$(DEPS)/include -I$(VPATH) -L$(DEPS)/lib -lspatialindex -pthread

control.o:
	$(CC) $(CFLAGS) -o $(BIN)/$@ -c $(CONTROLDIR)/control.c -I$(VPATH)

torus-node.o:
	$(CC) $(CFLAGS) -o $(BIN)/$@ -c $(TORUSDIR)/torus_node.c -I$(VPATH)
	
socket.o:
	$(CC) $(CFLAGS) -o $(BIN)/$@ -c $(SOCKETDIR)/socket.c -I$(VPATH)

skip-list.o:
	$(CC) $(CFLAGS) -o $(BIN)/$@ -c $(SKIPLISTDIR)/skip_list.c -I$(VPATH)
	
torus_rtree.o:
	$(CXX) $(CFLAGS) -o $(BIN)/$@ -c $(TORUSDIR)/torus_rtree.c -I$(VPATH) -I$(DEPS)/include -L$(DEPS)/lib -lspatialindex

server.o:
	$(CXX) $(CFLAGS) -o $(BIN)/$@ -c $(TORUSDIR)/server.c -I$(VPATH) -I$(DEPS)/include -L$(DEPS)/lib -lspatialindex -pthread
	
log.o:
	$(CC) $(CFLAGS) -o $(BIN)/$@ -c $(LOGDIR)/log.c -I$(VPATH)

test1:
	$(CC) -g -o $(BIN)/$@ test.c
	
.PHONY: clean
clean:
	-rm $(BIN)/*.o
	-rm $(BIN)/control $(BIN)/start-node $(BIN)/test1




