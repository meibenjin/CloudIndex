# this is makefile of torus

VPATH=../
OCTTREEDIR=../oct_tree
BIN=.
CC=gcc
CXX=g++
CFLAGS= -g -lrt -Wall -Wno-deprecated
#CFLAGS= -lrt -Wall -Wno-deprecated

all: oct_tree del_obj_file

oct_tree: main.o oct_tree.o oct_tree_node.o oct_tree_idx_node.o oct_tree_leaf_node.o oct_point.o utils.o
	$(CXX) $(CFLAGS) -o $(BIN)/$@ $(BIN)/main.o $(BIN)/oct_tree.o $(BIN)/oct_tree_node.o $(BIN)/oct_tree_idx_node.o $(BIN)/oct_tree_leaf_node.o $(BIN)/oct_point.o 

main.o: main.cpp
	$(CXX) $(CFLAGS) -o $(BIN)/$@ -c main.cpp -I$(VPATH)

utils.o:
	$(CC) $(CFLAGS) -o $(BIN)/$@ -c $(VPATH)/utils.c -I$(VPATH)

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

del_obj_file:
	-rm $(BIN)/*.o
	
.PHONY: clean
clean:
	-rm $(BIN)/*.o
	-rm $(BIN)/control $(BIN)/start-node $(BIN)/test
