/*
 * torus_node.h
 *
 *  Created on: Sep 16, 2013
 *      Author: meibenjin
 */

#ifndef TORUS_NODE_H_
#define TORUS_NODE_H_

#include"utils.h"

// limits for torus
#define MAX_NEIGHBORS 6
#define MAX_NODES_NUM 20 * 20 * 20

// torus node info 
typedef struct node_info {
	// torus node ip address
	char ip[IP_ADDR_LENGTH];

	// cluster id the node belongs to
	int cluster_id;

	// torus node coordinate
	struct coordinate node_id;

} node_info;

typedef struct torus_node {
	// torus node info
	struct node_info info;

	// number of neighbors
	int neighbors_num;

	// torus node's neighbors
	struct torus_node *neighbors[MAX_NEIGHBORS];
} torus_node;

torus_node the_torus_node;

// mark torus node is active or not 
int should_run;

void init_node_info(node_info *node_ptr);

// set torus node ip
void set_node_ip(node_info *node_ptr, const char *ip);
void get_node_ip(node_info node, char *ip);

// set the coordinate of a torus node
void set_node_id(node_info *node_ptr, int x, int y, int z);

struct coordinate get_node_id(node_info node);

void set_cluster_id(node_info *node_ptr, int cluster_id);

int get_cluster_id(node_info node);

// set the neighbors num of a torus node
void set_neighbors_num(torus_node *node_ptr, int neighbors_num);

int get_neighbors_num(torus_node node);

void print_neighbors(torus_node node);

void print_torus_node(torus_node torus);

void print_node_info(node_info node);

torus_node *new_torus_node();

#endif /* TORUS_NODE_H_ */

