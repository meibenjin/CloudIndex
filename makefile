# this is makefile of torus

SOCKETDIR=./socket
TORUSDIR=./torus_node
CONTROLDIR=./control_node
TESTDIR=./test
BIN=./bin

all: control start_node test1

control: control.o torus_node.o 
	gcc -g -o $(BIN)/$@ $(BIN)/control.o $(BIN)/torus_node.o

control.o:
	gcc -o $(BIN)/$@ -c $(CONTROLDIR)/control.c

torus_node.o:
	gcc -o $(BIN)/$@ -c $(TORUSDIR)/torus_node.c

start_node: start_node.o socket_server.o torus_node.o
	gcc -o $(BIN)/$@ $(BIN)/start_node.o $(BIN)/socket_server.o $(BIN)/torus_node.o 

start_node.o:
	gcc -o $(BIN)/$@ -c $(TORUSDIR)/start_node.c

socket_server.o:
	gcc -o $(BIN)/$@ -c $(SOCKETDIR)/socket_server.c

test1:
	gcc -o $(BIN)/$@ test.c
	
.PHONY: clean
clean:
	-rm $(BIN)/*
