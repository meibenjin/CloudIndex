# this is makefile of torus

VPATH=.
SRC_HOME=/home/meibenjin/Programming/C/CloudIndex
GSL_LIB=$(SRC_HOME)/deps/gsl/lib
GSL_INCLUDE=$(SRC_HOME)/deps/gsl/include
BIN=.
CC=gcc
CXX=g++
CFLAGS= -lrt -Wall -Wno-deprecated

all: gen_region 

gen_region: gen_region.o
	$(CC) $(CFLAGS) -o $(BIN)/$@ $(BIN)/gen_region.o -L$(GSL_LIB) -lgsl -lgslcblas -lm

gen_region.o:
	$(CC) $(CFLAGS) -o $(BIN)/$@ -c ./gen_region.c -I$(GSL_INCLUDE)
	
.PHONY: clean
clean:
	-rm $(BIN)/*.o
	-rm $(BIN)/gen_region




