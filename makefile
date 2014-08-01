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
SIMDIR=./simulation
SKIPLISTDIR=./skip_list
LOGDIR=./logs
CONFIGDIR=./config
TESTDIR=./test
BIN=./bin
CC=gcc
CXX=g++
#CFLAGS= -g -lrt -Wall -Wno-deprecated
CFLAGS= -lrt -Wall -Wno-deprecated

all: control start-node data_generator data_split query_split query_sim test data_partition load_data reload_properties del_obj_file 

control: utils.o control.o torus-node.o socket.o skip-list.o log.o config.o 
	$(CC) $(CFLAGS) -o $(BIN)/$@ $(BIN)/utils.o $(BIN)/control.o $(BIN)/torus-node.o $(BIN)/socket.o $(BIN)/skip-list.o $(BIN)/log.o  $(BIN)/config.o

start-node: utils.o config.o torus-node.o server.o socket.o skip-list.o log.o torus_rtree.o oct_tree.o oct_tree_node.o oct_tree_idx_node.o oct_tree_leaf_node.o oct_point.o 
	$(CXX) $(CFLAGS) $(BIN)/utils.o $(BIN)/config.o $(BIN)/torus-node.o $(BIN)/server.o $(BIN)/socket.o $(BIN)/skip-list.o $(BIN)/log.o $(BIN)/torus_rtree.o $(BIN)/oct_tree.o $(BIN)/oct_tree_node.o $(BIN)/oct_tree_idx_node.o $(BIN)/oct_tree_leaf_node.o $(BIN)/oct_point.o -o $(BIN)/$@ -L$(GSL_LIB) -L$(RTREE_LIB) -lspatialindex -lspatialindex_c -lgsl -lgslcblas -lm -pthread

data_generator: utils.o data_generator.o socket.o config.o torus-node.o log.o
	$(CC) $(CFLAGS) -o $(BIN)/$@ $(BIN)/utils.o $(BIN)/data_generator.o $(BIN)/socket.o $(BIN)/config.o $(BIN)/log.o $(BIN)/torus-node.o
data_generator.o: 
	$(CC) $(CFLAGS) -o $(BIN)/$@ -c $(DATAGENDIR)/generator.c -I$(VPATH)

data_split: utils.o data_split.o config.o torus-node.o log.o
	$(CC) $(CFLAGS) -o $(BIN)/$@ $(BIN)/utils.o $(BIN)/data_split.o $(BIN)/config.o $(BIN)/log.o $(BIN)/torus-node.o
data_split.o: 
	$(CC) $(CFLAGS) -o $(BIN)/$@ -c $(DATAGENDIR)/data_split.c -I$(VPATH)

load_data: utils.o load_data.o socket.o config.o torus-node.o log.o 
	$(CC) $(CFLAGS) -o $(BIN)/$@ $(BIN)/utils.o $(BIN)/load_data.o $(BIN)/socket.o $(BIN)/config.o $(BIN)/torus-node.o $(BIN)/log.o
load_data.o: 
	$(CC) $(CFLAGS) -o $(BIN)/$@ -c $(DATAGENDIR)/load_data.c -I$(VPATH)

reload_properties:utils.o reload_properties.o config.o socket.o torus-node.o 
	$(CC) $(CFLAGS) -o $(BIN)/$@ $(BIN)/utils.o $(BIN)/reload_properties.o $(BIN)/socket.o $(BIN)/config.o $(BIN)/torus-node.o 
reload_properties.o: 
	$(CC) $(CFLAGS) -o $(BIN)/$@ -c $(CONTROLDIR)/reload_properties.c -I$(VPATH)

data_partition: utils.o data_partition.o config.o torus-node.o log.o 
	$(CC) $(CFLAGS) -o $(BIN)/$@ $(BIN)/utils.o $(BIN)/data_partition.o $(BIN)/config.o $(BIN)/torus-node.o $(BIN)/log.o 
data_partition.o: 
	$(CC) $(CFLAGS) -o $(BIN)/$@ -c $(DATAGENDIR)/data_partition.c -I$(VPATH)

query_split: utils.o query_split.o 
	$(CC) $(CFLAGS) -o $(BIN)/$@ $(BIN)/utils.o $(BIN)/query_split.o
query_split.o: 
	$(CC) $(CFLAGS) -o $(BIN)/$@ -c $(DATAGENDIR)/query_split.c -I$(VPATH)

control.o:
	$(CC) $(CFLAGS) -o $(BIN)/$@ -c $(CONTROLDIR)/control.c -I$(VPATH)

torus-node.o:
	$(CC) $(CFLAGS) -o $(BIN)/$@ -c $(TORUSDIR)/torus_node.c -I$(VPATH)
	
socket.o:
	$(CC) $(CFLAGS) -o $(BIN)/$@ -c $(SOCKETDIR)/socket.c -I$(VPATH)

utils.o:
	$(CC) $(CFLAGS) -o $(BIN)/$@ -c $(VPATH)/utils.c -I$(VPATH)

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

query_sim: utils.o socket.o query_sim.o
	$(CC) $(CFLAGS) -o $(BIN)/$@ $(BIN)/utils.o $(BIN)/socket.o $(BIN)/query_sim.o 

query_sim.o: 
	$(CC) $(CFLAGS) -o $(BIN)/$@ -c $(SIMDIR)/query_sim.c -I$(VPATH)

test: utils.o socket.o test.o
	$(CC) $(CFLAGS) -o $(BIN)/$@ $(BIN)/utils.o $(BIN)/socket.o $(BIN)/test.o -L$(GSL_LIB) -lgsl -lgslcblas -lm

test.o: 
	$(CC) $(CFLAGS) -o $(BIN)/$@ -c test.c -I$(VPATH) -I$(GSL_INCLUDE) 

del_obj_file:
	-rm $(BIN)/*.o
	
.PHONY: clean
clean:
	-rm $(BIN)/*.o
	-rm $(BIN)/control $(BIN)/start-node $(BIN)/test
