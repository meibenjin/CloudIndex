# this is makefile of torus

VPATH=.
SOCKETDIR=./socket
TORUSDIR=./torus_node
CONTROLDIR=./control_node
SKIPLISTDIR=./skip_list
LOGDIR=./logs
TESTDIR=./test
BIN=./bin
CC=gcc

all: control start-node test1

control: control.o torus-node.o socket.o skip-list.o log.o
	$(CC) -g -o $(BIN)/$@ $(BIN)/control.o $(BIN)/torus-node.o $(BIN)/socket.o $(BIN)/skip-list.o $(BIN)/log.o

start-node: start-node.o torus-node.o server.o socket.o skip-list.o log.o
	$(CC) -g -o $(BIN)/$@ $(BIN)/start-node.o $(BIN)/torus-node.o $(BIN)/server.o $(BIN)/socket.o $(BIN)/skip-list.o $(BIN)/log.o
	 
control.o:
	$(CC) -g -o $(BIN)/$@ -c $(CONTROLDIR)/control.c -I$(VPATH)

torus-node.o:
	$(CC) -g -o $(BIN)/$@ -c $(TORUSDIR)/torus_node.c -I$(VPATH)
	
socket.o:
	$(CC) -g -o $(BIN)/$@ -c $(SOCKETDIR)/socket.c -I$(VPATH)

skip-list.o:
	$(CC) -g -o $(BIN)/$@ -c $(SKIPLISTDIR)/skip_list.c -I$(VPATH)
	
start-node.o:
	$(CC) -g -o $(BIN)/$@ -c $(TORUSDIR)/start_node.c -I$(VPATH)

server.o:
	$(CC) -g -o $(BIN)/$@ -c $(TORUSDIR)/server.c -I$(VPATH)
	
log.o:
	$(CC) -g -o $(BIN)/$@ -c $(LOGDIR)/log.c -I$(VPATH)

test1:
	$(CC) -g -o $(BIN)/$@ test.c
	
.PHONY: clean
clean:
	-rm $(BIN)/*.o
	-rm $(BIN)/control $(BIN)/start-node $(BIN)/test1




