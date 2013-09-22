# this is makefile of torus

SOCKETDIR=./socket
TORUSDIR=./torus_node
CONTROLDIR=./control_node
TESTDIR=./test
BIN=./bin

all: control start_node

control: control.o torus_node.o socket.o
	gcc -g -o $(BIN)/$@ $(BIN)/control.o $(BIN)/torus_node.o $(BIN)/socket.o

control.o:
	gcc -g -o $(BIN)/$@ -c $(CONTROLDIR)/control.c

torus_node.o:
	gcc -g -o $(BIN)/$@ -c $(TORUSDIR)/torus_node.c

start_node: start_node.o torus_node.o socket.o
	gcc -g -o $(BIN)/$@ $(BIN)/start_node.o $(BIN)/torus_node.o $(BIN)/socket.o 

start_node.o:
	gcc -g -o $(BIN)/$@ -c $(TORUSDIR)/start_node.c

socket.o:
	gcc -g -o $(BIN)/$@ -c $(SOCKETDIR)/socket.c

test1:
	gcc -g -o $(BIN)/$@ test.c
	
.PHONY: clean
clean:
	-rm $(BIN)/*
