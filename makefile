# this is makefile of torus

SOCKETDIR=./socket
TESTDIR=./test
BIN=./bin

all: torus start_server connect_server test1

torus:
	gcc -g -o $(BIN)/$@ torus_node.c

start_server: start_server.o socket_server.o 
	gcc -o $(BIN)/$@ $(BIN)/start_server.o $(BIN)/socket_server.o 
socket_server.o:
	gcc -o $(BIN)/$@ -c $(SOCKETDIR)/socket_server.c 
start_server.o:
	gcc -o $(BIN)/$@ -c $(TESTDIR)/start_server_test.c

connect_server: connect_server.o 
	gcc -o $(BIN)/$@ $(BIN)/connect_server.o 
connect_server.o: 
	gcc -o $(BIN)/$@ -c $(TESTDIR)/connect_server_test.c
	
test1:
	gcc -o $(BIN)/$@ test.c
	
.PHONY: clean
clean:
	-rm $(BIN)/*
