/*
 * control.h
 *
 *  Created on: Sep 20, 2013
 *      Author: meibenjin
 */

#ifndef CONTROL_H_
#define CONTROL_H_

#include"utils.h"
#include"torus_node/torus_node.h"
#include"socket/socket.h"
#include"skip_list/skip_list.h"

#define TORUS_IP_LIST "../etc/torus_ip_list"

// directions code in 3-dimension
enum direction
{
    X_R = 0,
    X_L,
    Y_R,
    Y_L,
    Z_R,
    Z_L,
};

// torus partitions on 3-dimension
typedef struct torus_partitions {
	int p_x;
	int p_y;
	int p_z;
} torus_partitions;

int set_partitions(int p_x, int p_y, int p_z);

void update_partitions(int direction, struct torus_partitions t_p);

// calcate the total torus nodes number 
int get_nodes_num(struct torus_partitions t_p);

int translate_coordinates(int direction, struct torus_partitions t_p, struct torus_node *node_list);

// assign torus node ip address
int assign_node_ip(torus_node *node_ptr);

int assign_cluster_id(torus_node *node_ptr);

// return the index of torus node in torus node list
int get_node_index(int x, int y, int z);

int set_neighbors(torus_node *node_ptr);

// create a new torus
int new_torus(struct torus_partitions new_torus_p, struct torus_node *new_list);

/* construct a newly created torus
 * construct steps:
 *      create a new torus 
 *      set global variable torus_node_list
 *      set neighbors for every torus node
 */
int construct_torus();

// append a extra new torus
int append_torus(int direction);

// merge appent_list into torus_node_list
int merge_torus(int direction, struct torus_partitions append_torus_p, struct torus_node *append_list);

// send torus nodes info to dst_ip
int send_torus_nodes(const char *dst_ip, int nodes_num, struct node_info *nodes);

// update all nodes and theirs neighbors info via socket
int update_torus();

// traverse the torus from entry ip
int traverse_torus(const char *entry_ip);

void print_torus();

int create_skip_list();

int read_torus_ip_list();

#endif /* CONTROL_H_ */

