# this is makefile of torus

SRC_HOME=/home/meibenjin/Programming/C/CloudIndex
VPATH=$(SRC_HOME)/
GSL_LIB=$(SRC_HOME)/deps/gsl/lib
GSL_INCLUDE=$(SRC_HOME)/deps/gsl/include
BIN=$(SRC_HOME)/bin
CC=gcc
CXX=g++
CFLAGS= -g -lrt -Wall -Wno-deprecated

all: gen_nn_query

gen_nn_query: gen_nn_query.o
	$(CC) $(CFLAGS) -o $(BIN)/$@ $(BIN)/gen_nn_query.o -L$(GSL_LIB) -lgsl -lgslcblas -lm

gen_nn_query.o:
	$(CC) $(CFLAGS) -o $(BIN)/$@ -c $(SRC_HOME)/test/nn_query/gen_query.c -I$(GSL_INCLUDE) -I$(VPATH)
	
.PHONY: clean
clean:
	-rm $(BIN)/*.o
	-rm $(BIN)/gen_nn_query




