/*
 * control.h
 *
 *  Created on: Sep 20, 2013
 *      Author: meibenjin
 */

#ifndef CONTROL_H_
#define CONTROL_H_

#include<stdio.h>
#include<stdlib.h>
#include<string.h>


#include"../utils.h"
#include"../torus_node/torus_node.h"
#include"../socket/socket.h"

// torus partitions on 3-dimension
typedef struct torus_partitions {
	int p_x;
	int p_y;
	int p_z;
} torus_partitions;

// torus node list
torus_node *torus_node_list;
int torus_node_num;

// partitions on 3-dimension
torus_partitions torus_p;

int set_partitions(int p_x, int p_y, int p_z);

// assign torus node ip address
int assign_node_ip(torus_node *node_ptr);

// return the index of torus node in torus node list
int get_node_index(int x, int y, int z);

int set_neighbors(torus_node *node_ptr, int x, int y, int z);

// create a new torus
int create_torus();

void print_torus();

// send torus nodes info to dst_ip
int send_torus_nodes(const char *dst_ip, int nodes_num, struct node_info *nodes);

// update all nodes and theirs neighbors info via socket
int update_torus();

int traverse_torus(const char *entry_ip);

#endif /* CONTROL_H_ */

