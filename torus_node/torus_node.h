/*
 * torus_node.h
 *
 *  Created on: Sep 16, 2013
 *      Author: meibenjin
 */

#ifndef TORUS_NODE_H_
#define TORUS_NODE_H_

#include"utils.h"


void init_node_info(node_info *node_ptr);

void init_torus_node(torus_node *node_ptr);

int set_interval(node_info *node_ptr);

// set torus node's capacity
void set_node_capacity(node_info *info_ptr, int c);

int get_node_capacity(node_info info);

// set torus node ip
void set_node_ip(node_info *node_ptr, const char *ip);

void get_node_ip(node_info node, char *ip);

// set the coordinate of a torus node
void set_node_id(node_info *node_ptr, int x, int y, int z);

struct coordinate get_node_id(node_info node);

void set_cluster_id(node_info *node_ptr, int cluster_id);

int get_cluster_id(node_info node);

// add neighbor info 
int add_neighbor_info(torus_node *node_ptr, int d, node_info *neighbor_ptr);

// set the neighbors num of a torus node
void set_neighbors_num(torus_node *node_ptr, int neighbors_num);

// get neighbors num at direction d
int get_neighbors_num_d(torus_node *node_ptr, int d);

// get neighbors num of torus node 
int get_neighbors_num(torus_node *node_ptr);

node_info *get_neighbor_by_id(torus_node *node_ptr, struct coordinate node_id);

void print_neighbors(torus_node node);

void print_torus_node(torus_node torus);

extern void print_node_info(node_info node);

torus_node *new_torus_node();

#endif /* TORUS_NODE_H_ */

