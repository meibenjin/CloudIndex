# this is makefile of torus

VPATH=.
RTREE_LIB=./deps/spatialindex/lib
RTREE_INCLUDE=./deps/spatialindex/include
GSL_LIB=./deps/gsl/lib
GSL_INCLUDE=./deps/gsl/include
SOCKETDIR=./socket
TORUSDIR=./torus_node
OCTTREEDIR=./oct_tree
CONTROLDIR=./control_node
SKIPLISTDIR=./skip_list
LOGDIR=./logs
TESTDIR=./test
BIN=./bin
CC=gcc
CXX=g++
CFLAGS= -lrt -Wall -Wno-deprecated

all: control start-node test1

control: control.o torus-node.o socket.o skip-list.o log.o
	$(CC) $(CFLAGS) -o $(BIN)/$@ $(BIN)/control.o $(BIN)/torus-node.o $(BIN)/socket.o $(BIN)/skip-list.o $(BIN)/log.o 

start-node:torus-node.o server.o socket.o skip-list.o log.o torus_rtree.o oct_tree.o oct_tree_node.o oct_tree_idx_node.o oct_tree_leaf_node.o oct_point.o 
	$(CXX) $(CFLAGS) $(BIN)/torus-node.o $(BIN)/server.o $(BIN)/socket.o $(BIN)/skip-list.o $(BIN)/log.o $(BIN)/torus_rtree.o $(BIN)/oct_tree.o $(BIN)/oct_tree_node.o $(BIN)/oct_tree_idx_node.o $(BIN)/oct_tree_leaf_node.o $(BIN)/oct_point.o -o $(BIN)/$@ -I$(VPATH) -I$(RTREE_INCLUDE) -I$(GSL_INCLUDE) -L$(GSL_LIB) -L$(RTREE_LIB) -lspatialindex -lspatialindex_c -lgsl -lgslcblas -lm -pthread

control.o:
	$(CC) $(CFLAGS) -o $(BIN)/$@ -c $(CONTROLDIR)/control.c -I$(VPATH)

torus-node.o:
	$(CC) $(CFLAGS) -o $(BIN)/$@ -c $(TORUSDIR)/torus_node.c -I$(VPATH)
	
socket.o:
	$(CC) $(CFLAGS) -o $(BIN)/$@ -c $(SOCKETDIR)/socket.c -I$(VPATH)

skip-list.o:
	$(CC) $(CFLAGS) -o $(BIN)/$@ -c $(SKIPLISTDIR)/skip_list.c -I$(VPATH)
	
torus_rtree.o:
	$(CXX) $(CFLAGS) -o $(BIN)/$@ -c $(TORUSDIR)/torus_rtree.c -I$(VPATH) -I$(RTREE_INCLUDE) -L$(RTREE_LIB) -lspatialindex 

server.o:
	$(CXX) $(CFLAGS) -o $(BIN)/$@ -c $(TORUSDIR)/server.c -I$(VPATH) -I$(RTREE_INCLUDE) -I$(GSL_INCLUDE) -L$(GSL_LIB) -L$(RTREE_LIB) -lspatialindex -lspatialindex_c -lgsl -lgslcblas -lm -pthread
	
log.o:
	$(CC) $(CFLAGS) -o $(BIN)/$@ -c $(LOGDIR)/log.c -I$(VPATH) 

oct_tree.o:
	$(CXX) $(CFLAGS) -o $(BIN)/$@ -c $(OCTTREEDIR)/OctTree.cpp -I$(VPATH)

oct_tree_node.o:
	$(CXX) $(CFLAGS) -o $(BIN)/$@ -c $(OCTTREEDIR)/OctTNode.cpp -I$(VPATH)

oct_tree_idx_node.o:
	$(CXX) $(CFLAGS) -o $(BIN)/$@ -c $(OCTTREEDIR)/OctIdxNode.cpp -I$(VPATH)

oct_tree_leaf_node.o:
	$(CXX) $(CFLAGS) -o $(BIN)/$@ -c $(OCTTREEDIR)/OctLeafNode.cpp -I$(VPATH)

oct_point.o:
	$(CXX) $(CFLAGS) -o $(BIN)/$@ -c $(OCTTREEDIR)/Point.cpp -I$(VPATH)

test1: socket.o test1.o
	$(CC) $(CFLAGS) -o $(BIN)/$@ $(BIN)/socket.o $(BIN)/test1.o -I$(GSL_INCLUDE) -L$(GSL_LIB) -lgsl -lgslcblas -lm

test1.o: 
	$(CC) $(CFLAGS) -o $(BIN)/$@ -c test.c -I$(VPATH) -I$(GSL_INCLUDE) -L$(GSL_LIB) -lgsl -lgslcblas -lm
	
.PHONY: clean
clean:
	-rm $(BIN)/*.o
	-rm $(BIN)/control $(BIN)/start-node $(BIN)/test1




