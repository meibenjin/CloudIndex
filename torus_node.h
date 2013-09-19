/*
 * torus_node.h
 *
 *  Created on: Sep 16, 2013
 *      Author: meibenjin
 */

#ifndef TORUS_NODE_H_
#define TORUS_NODE_H_

#include"utils.h"
#include"socket/socket_def.h"

// torus partitions on 3-dimension
typedef struct torus_partitions
{
	int p_x;
	int p_y;
	int p_z;
}torus_partitions;

typedef struct torus_node
{
	// torus node ip address
	char node_ip[MAX_IP_ADDR_LENGTH];

	// torus node coordinate
	struct coordinate node_id;

    // number of neighbors
    int neighbors_num;

	// torus node's neighbors 
	struct torus_node *neighbors[MAX_NEIGHBORS];


}torus_node;

// torus node list
torus_node *torus_node_list;
int torus_node_num;

// partitions on 3-dimension
torus_partitions torus_p;

int set_partitions(int p_x, int p_y, int p_z);

// assign torus node ip address
int assign_node_ip(torus_node *node_ptr);

// set the coordinate of a torus node
void set_coordinate(torus_node *node_ptr, int x, int y, int z);

void print_coordinate(torus_node node);

// return the index of torus node in torus node list
int get_node_index(int x, int y, int z);

int set_neighbors(torus_node *node_ptr, int x, int y, int z);

void print_neighbors(torus_node node);

int init_torus_node(torus_node *node_ptr);

// create a new torus
int create_torus();

void print_torus();

// dispatch all nodes and theirs neighbors info via socket
int dispatch_torus_nodes();


#endif /* TORUS_NODE_H_ */


