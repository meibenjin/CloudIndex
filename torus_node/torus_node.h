#ifndef TORUS_NODE_H_
#define TORUS_NODE_H_

/*
 * torus_node.h
 *
 *  Created on: Sep 16, 2013
 *      Author: meibenjin
 */

#include"utils.h"

torus_node the_torus_node;
struct torus_partitions the_partition;

// mark torus node is active or not 
int should_run;

skip_list the_skip_list;

void init_node_info(node_info *node_ptr);

extern void init_torus_node(torus_node *node_ptr);

int assign_dimensions(node_info *node_ptr);

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

node_info *get_neighbor_by_id(torus_node node, struct coordinate node_id);

void print_neighbors(torus_node node);

void print_torus_node(torus_node torus);

extern void print_node_info(node_info node);

torus_node *new_torus_node();

#endif /* TORUS_NODE_H_ */

