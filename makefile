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
DATAGENDIR=./data_generator
SKIPLISTDIR=./skip_list
LOGDIR=./logs
CONFIGDIR=./config
TESTDIR=./test
BIN=./bin
CC=gcc
CXX=g++
CFLAGS= -g -lrt -Wall -Wno-deprecated

all: control start-node data_generator data_split query_split test1 del_obj_file

control: control.o torus-node.o socket.o skip-list.o log.o config.o
	$(CC) $(CFLAGS) -o $(BIN)/$@ $(BIN)/control.o $(BIN)/torus-node.o $(BIN)/socket.o $(BIN)/skip-list.o $(BIN)/log.o  $(BIN)/config.o

#start-node:torus-node.o server.o socket.o skip-list.o log.o torus_rtree.o oct_tree.o oct_tree_node.o oct_tree_idx_node.o oct_tree_leaf_node.o oct_point.o 
	#$(CXX) $(CFLAGS) $(BIN)/torus-node.o $(BIN)/server.o $(BIN)/socket.o $(BIN)/skip-list.o $(BIN)/log.o $(BIN)/torus_rtree.o $(BIN)/oct_tree.o $(BIN)/oct_tree_node.o $(BIN)/oct_tree_idx_node.o $(BIN)/oct_tree_leaf_node.o $(BIN)/oct_point.o -o $(BIN)/$@ -I$(VPATH) -I$(RTREE_INCLUDE) -I$(GSL_INCLUDE) -L$(GSL_LIB) -L$(RTREE_LIB) -lspatialindex -lspatialindex_c -lgsl -lgslcblas -lm -pthread

start-node: torus-node.o server.o socket.o skip-list.o log.o torus_rtree.o oct_tree.o oct_tree_node.o oct_tree_idx_node.o oct_tree_leaf_node.o oct_point.o 
	$(CXX) $(CFLAGS) $(BIN)/torus-node.o $(BIN)/server.o $(BIN)/socket.o $(BIN)/skip-list.o $(BIN)/log.o $(BIN)/torus_rtree.o $(BIN)/oct_tree.o $(BIN)/oct_tree_node.o $(BIN)/oct_tree_idx_node.o $(BIN)/oct_tree_leaf_node.o $(BIN)/oct_point.o -o $(BIN)/$@ -L$(GSL_LIB) -L$(RTREE_LIB) -lspatialindex -lspatialindex_c -lgsl -lgslcblas -lm -pthread

data_generator: data_generator.o socket.o config.o torus-node.o log.o
	$(CC) $(CFLAGS) -o $(BIN)/$@ $(BIN)/data_generator.o $(BIN)/socket.o $(BIN)/config.o $(BIN)/log.o $(BIN)/torus-node.o

data_split: data_split.o config.o torus-node.o log.o
	$(CC) $(CFLAGS) -o $(BIN)/$@ $(BIN)/data_split.o $(BIN)/config.o $(BIN)/log.o $(BIN)/torus-node.o

query_split: query_split.o 
	$(CC) $(CFLAGS) -o $(BIN)/$@ $(BIN)/query_split.o

data_generator.o: 
	$(CC) $(CFLAGS) -o $(BIN)/$@ -c $(DATAGENDIR)/generator.c -I$(VPATH)

data_split.o: 
	$(CC) $(CFLAGS) -o $(BIN)/$@ -c $(DATAGENDIR)/data_split.c -I$(VPATH)

query_split.o: 
	$(CC) $(CFLAGS) -o $(BIN)/$@ -c $(DATAGENDIR)/query_split.c -I$(VPATH)

control.o:
	$(CC) $(CFLAGS) -o $(BIN)/$@ -c $(CONTROLDIR)/control.c -I$(VPATH)

torus-node.o:
	$(CC) $(CFLAGS) -o $(BIN)/$@ -c $(TORUSDIR)/torus_node.c -I$(VPATH)
	
socket.o:
	$(CC) $(CFLAGS) -o $(BIN)/$@ -c $(SOCKETDIR)/socket.c -I$(VPATH)

skip-list.o:
	$(CC) $(CFLAGS) -o $(BIN)/$@ -c $(SKIPLISTDIR)/skip_list.c -I$(VPATH)
	
torus_rtree.o:
	$(CXX) $(CFLAGS) -o $(BIN)/$@ -c $(TORUSDIR)/torus_rtree.c -I$(VPATH) -I$(RTREE_INCLUDE) 
	#-L$(RTREE_LIB) -lspatialindex 

server.o:
	$(CXX) $(CFLAGS) -o $(BIN)/$@ -c $(TORUSDIR)/server.c -I$(VPATH) -I$(RTREE_INCLUDE) -I$(GSL_INCLUDE) 
	#-L$(GSL_LIB) -L$(RTREE_LIB) -lspatialindex -lspatialindex_c -lgsl -lgslcblas -lm -pthread
	
log.o:
	$(CC) $(CFLAGS) -o $(BIN)/$@ -c $(LOGDIR)/log.c -I$(VPATH) 

config.o:
	$(CC) $(CFLAGS) -o $(BIN)/$@ -c $(CONFIGDIR)/config.c -I$(VPATH) 


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
	$(CC) $(CFLAGS) -o $(BIN)/$@ $(BIN)/socket.o $(BIN)/test1.o -L$(GSL_LIB) -lgsl -lgslcblas -lm

test1.o: 
	$(CC) $(CFLAGS) -o $(BIN)/$@ -c test.c -I$(VPATH) -I$(GSL_INCLUDE) 
	#-L$(GSL_LIB) -lgsl -lgslcblas -lm

del_obj_file:
	-rm $(BIN)/*.o
	
.PHONY: clean
clean:
	-rm $(BIN)/*.o
	-rm $(BIN)/control $(BIN)/start-node $(BIN)/test1




