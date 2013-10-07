# this is makefile of torus

VPATH=.
SOCKETDIR=./socket
TORUSDIR=./torus_node
CONTROLDIR=./control_node
SKIPLISTDIR=./skip_list
TESTDIR=./test
BIN=./bin

all: control start_node test1

control: control.o torus_node.o socket.o skip_list.o
	gcc -g -o $(BIN)/$@ $(BIN)/control.o $(BIN)/torus_node.o $(BIN)/socket.o $(BIN)/skip_list.o

start_node: start_node.o torus_node.o server.o socket.o
	gcc -g -o $(BIN)/$@ $(BIN)/start_node.o $(BIN)/torus_node.o $(BIN)/server.o $(BIN)/socket.o
	 
control.o:
	gcc -g -o $(BIN)/$@ -c $(CONTROLDIR)/control.c -I$(VPATH)

torus_node.o:
	gcc -g -o $(BIN)/$@ -c $(TORUSDIR)/torus_node.c -I$(VPATH)
	
socket.o:
	gcc -g -o $(BIN)/$@ -c $(SOCKETDIR)/socket.c -I$(VPATH)

skip_list.o:
	gcc -g -o $(BIN)/$@ -c $(SKIPLISTDIR)/skip_list.c -I$(VPATH)
	
start_node.o:
	gcc -g -o $(BIN)/$@ -c $(TORUSDIR)/start_node.c -I$(VPATH)

server.o:
	gcc -g -o $(BIN)/$@ -c $(TORUSDIR)/server.c -I$(VPATH)


test1:
	gcc -g -o $(BIN)/$@ test.c
	
.PHONY: clean
clean:
	-rm $(BIN)/*



