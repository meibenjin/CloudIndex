# this is makefile of torus

VPATH=.
RTREE_LIB=./deps/spatialindex/lib
RTREE_INCLUDE=./deps/spatialindex/include
GSL_LIB=./deps/gsl/lib
GSL_INCLUDE=./deps/gsl/include
CQUEUEDIR=./deps/cqueue/
UTILSDIR=./utils
SOCKETDIR=./communication
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
CFLAGS= -g -lrt -Wall -Wno-deprecated
#CFLAGS= -lrt -Wall -Wno-deprecated

all: control start-node data_generator data_split query_split query_sim data_partition msra_data_partition load_data test_system_status test_insert_data test_range_nn_query test_conn_buffer test_traj_seq reload_properties del_obj_file 

control:log.o utils.o geometry.o control.o torus-node.o socket.o message.o skip-list.o config.o 
	$(CC) $(CFLAGS) -o $(BIN)/$@ $(BIN)/utils.o $(BIN)/geometry.o $(BIN)/control.o $(BIN)/torus-node.o $(BIN)/socket.o $(BIN)/message.o $(BIN)/skip-list.o $(BIN)/log.o  $(BIN)/config.o -lm

start-node:log.o utils.o geometry.o config.o torus-node.o server.o socket.o message.o conn_manager.o skip-list.o torus_rtree.o oct_tree.o oct_tree_node.o oct_tree_idx_node.o oct_tree_leaf_node.o oct_point.o 
	$(CXX) $(CFLAGS) $(BIN)/utils.o $(BIN)/geometry.o $(BIN)/config.o $(BIN)/torus-node.o $(BIN)/server.o $(BIN)/socket.o $(BIN)/message.o $(BIN)/conn_manager.o $(BIN)/skip-list.o $(BIN)/log.o $(BIN)/torus_rtree.o $(BIN)/oct_tree.o $(BIN)/oct_tree_node.o $(BIN)/oct_tree_idx_node.o $(BIN)/oct_tree_leaf_node.o $(BIN)/oct_point.o -o $(BIN)/$@ -L$(GSL_LIB) -L$(RTREE_LIB) -lspatialindex -lspatialindex_c -lgsl -lgslcblas -lm -pthread

data_generator:log.o utils.o data_generator.o socket.o message.o config.o torus-node.o 
	$(CC) $(CFLAGS) -o $(BIN)/$@ $(BIN)/utils.o $(BIN)/data_generator.o $(BIN)/socket.o $(BIN)/message.o $(BIN)/config.o $(BIN)/log.o $(BIN)/torus-node.o
data_generator.o: 
	$(CC) $(CFLAGS) -o $(BIN)/$@ -c $(DATAGENDIR)/generator.c -I$(VPATH)

data_split:log.o utils.o data_split.o config.o torus-node.o
	$(CC) $(CFLAGS) -o $(BIN)/$@ $(BIN)/utils.o $(BIN)/data_split.o $(BIN)/config.o $(BIN)/log.o $(BIN)/torus-node.o
data_split.o: 
	$(CC) $(CFLAGS) -o $(BIN)/$@ -c $(DATAGENDIR)/data_split.c -I$(VPATH)

load_data:log.o utils.o load_data.o socket.o message.o config.o torus-node.o  
	$(CC) $(CFLAGS) -o $(BIN)/$@ $(BIN)/utils.o $(BIN)/load_data.o $(BIN)/socket.o $(BIN)/message.o $(BIN)/config.o $(BIN)/torus-node.o $(BIN)/log.o
load_data.o: 
	$(CC) $(CFLAGS) -o $(BIN)/$@ -c $(DATAGENDIR)/load_data.c -I$(VPATH)

reload_properties:utils.o reload_properties.o config.o socket.o message.o torus-node.o 
	$(CC) $(CFLAGS) -o $(BIN)/$@ $(BIN)/utils.o $(BIN)/reload_properties.o $(BIN)/socket.o $(BIN)/message.o $(BIN)/config.o $(BIN)/torus-node.o 
reload_properties.o: 
	$(CC) $(CFLAGS) -o $(BIN)/$@ -c $(CONTROLDIR)/reload_properties.c -I$(VPATH)

data_partition:log.o utils.o data_partition.o config.o torus-node.o  
	$(CC) $(CFLAGS) -o $(BIN)/$@ $(BIN)/utils.o $(BIN)/data_partition.o $(BIN)/config.o $(BIN)/torus-node.o $(BIN)/log.o 
data_partition.o: 
	$(CC) $(CFLAGS) -o $(BIN)/$@ -c $(DATAGENDIR)/data_partition.c -I$(VPATH)

msra_data_partition:log.o utils.o geometry.o msra_data_partition.o config.o torus-node.o 
	$(CC) $(CFLAGS) -o $(BIN)/$@ $(BIN)/utils.o $(BIN)/geometry.o $(BIN)/msra_data_partition.o $(BIN)/config.o $(BIN)/torus-node.o $(BIN)/log.o -lm
msra_data_partition.o: 
	$(CC) $(CFLAGS) -o $(BIN)/$@ -c $(DATAGENDIR)/msra_data_partition.c -I$(VPATH)

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

message.o:
	$(CC) $(CFLAGS) -o $(BIN)/$@ -c $(SOCKETDIR)/message.c -I$(VPATH)

conn_manager.o:
	$(CC) $(CFLAGS) -o $(BIN)/$@ -c $(SOCKETDIR)/connection_mgr.c -I$(VPATH)

cqueue.o:
	$(CC) $(CFLAGS) -o $(BIN)/$@ -c $(CQUEUEDIR)/cqueue.c -I$(VPATH)

utils.o:
	$(CC) $(CFLAGS) -o $(BIN)/$@ -c $(VPATH)/utils.c -I$(VPATH)

geometry.o:
	$(CC) $(CFLAGS) -o $(BIN)/$@ -c $(UTILSDIR)/geometry.c -I$(VPATH)

skip-list.o:
	$(CC) $(CFLAGS) -o $(BIN)/$@ -c $(SKIPLISTDIR)/skip_list.c -I$(VPATH)
	
torus_rtree.o:
	$(CXX) $(CFLAGS) -o $(BIN)/$@ -c $(TORUSDIR)/torus_rtree.c -I$(VPATH) -I$(RTREE_INCLUDE) 

server.o:
	$(CXX) $(CFLAGS) -o $(BIN)/$@ -c $(TORUSDIR)/server.c -I$(VPATH) -I$(RTREE_INCLUDE) -I$(GSL_INCLUDE) 
	
log.o:
	$(CC) $(CFLAGS) -o $(BIN)/$@ -c $(LOGDIR)/log.c -I$(VPATH) 

config.o:
	$(CC) $(CFLAGS) -o $(BIN)/$@ -c $(CONFIGDIR)/config.c -I$(VPATH) 

oct_tree.o:
	$(CXX) $(CFLAGS) -o $(BIN)/$@  -c $(OCTTREEDIR)/OctTree.cpp -I$(VPATH)

oct_tree_node.o:
	$(CXX) $(CFLAGS) -o $(BIN)/$@ -c $(OCTTREEDIR)/OctTNode.cpp -I$(VPATH)

oct_tree_idx_node.o:
	$(CXX) $(CFLAGS) -o $(BIN)/$@ -c $(OCTTREEDIR)/OctIdxNode.cpp -I$(VPATH)

oct_tree_leaf_node.o:
	$(CXX) $(CFLAGS) -o $(BIN)/$@ -c $(OCTTREEDIR)/OctLeafNode.cpp -I$(VPATH)

oct_point.o: 
	$(CXX) $(CFLAGS) -o $(BIN)/$@ -c $(OCTTREEDIR)/Point.cpp -I$(VPATH)

query_sim: utils.o socket.o message.o query_sim.o
	$(CC) $(CFLAGS) -o $(BIN)/$@ $(BIN)/utils.o $(BIN)/socket.o $(BIN)/message.o $(BIN)/query_sim.o 

query_sim.o: 
	$(CC) $(CFLAGS) -o $(BIN)/$@ -c $(SIMDIR)/query_sim.c -I$(VPATH)

#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#test case

test_system_status:log.o utils.o test_system_status.o socket.o message.o config.o torus-node.o  
	$(CC) $(CFLAGS) -o $(BIN)/$@ $(BIN)/utils.o $(BIN)/test_system_status.o $(BIN)/socket.o $(BIN)/message.o $(BIN)/config.o $(BIN)/torus-node.o $(BIN)/log.o
test_system_status.o: 
	$(CC) $(CFLAGS) -o $(BIN)/$@ -c $(TESTDIR)/test_system_status.c -I$(VPATH)

test_insert_data:log.o utils.o test_insert_data.o socket.o message.o config.o torus-node.o  
	$(CC) $(CFLAGS) -o $(BIN)/$@ $(BIN)/utils.o $(BIN)/test_insert_data.o $(BIN)/socket.o $(BIN)/message.o $(BIN)/config.o $(BIN)/torus-node.o $(BIN)/log.o
test_insert_data.o: 
	$(CC) $(CFLAGS) -o $(BIN)/$@ -c $(TESTDIR)/test_insert_data.c -I$(VPATH)

test_range_nn_query: utils.o socket.o message.o test_range_nn_query.o
	$(CC) $(CFLAGS) -o $(BIN)/$@ $(BIN)/utils.o $(BIN)/socket.o $(BIN)/message.o $(BIN)/test_range_nn_query.o 
test_range_nn_query.o: 
	$(CC) $(CFLAGS) -o $(BIN)/$@ -c $(TESTDIR)/test_range_nn_query.c -I$(VPATH)

test_conn_buffer:log.o utils.o test_conn_buffer.o socket.o message.o config.o torus-node.o  
	$(CC) $(CFLAGS) -o $(BIN)/$@ $(BIN)/utils.o $(BIN)/test_conn_buffer.o $(BIN)/socket.o $(BIN)/message.o $(BIN)/config.o $(BIN)/torus-node.o $(BIN)/log.o
test_conn_buffer.o: 
	$(CC) $(CFLAGS) -o $(BIN)/$@ -c $(TESTDIR)/test_conn_buffer.c -I$(VPATH)

test_traj_seq:log.o utils.o test_traj_seq.o socket.o message.o config.o torus-node.o  
	$(CC) $(CFLAGS) -o $(BIN)/$@ $(BIN)/utils.o $(BIN)/test_traj_seq.o $(BIN)/socket.o $(BIN)/message.o $(BIN)/config.o $(BIN)/torus-node.o $(BIN)/log.o
test_traj_seq.o: 
	$(CC) $(CFLAGS) -o $(BIN)/$@ -c $(TESTDIR)/test_traj_seq.c -I$(VPATH)


del_obj_file:
	-rm $(BIN)/*.o
	
.PHONY: clean
clean:
	-rm $(BIN)/*.o
	-rm $(BIN)/control $(BIN)/start-node
