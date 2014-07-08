/*
 * control.h
 *
 *  Created on: Sep 20, 2013
 *      Author: meibenjin
 */

#ifndef CONTROL_H_
#define CONTROL_H_

#include"utils.h"

torus_cluster *new_torus_cluster();

torus_cluster *find_torus_cluster(torus_cluster *list, int cluster_id);

torus_cluster *insert_torus_cluster(torus_cluster *list, torus_s *torus_ptr);

int remove_torus_cluster(torus_cluster *list, int cluster_id);

int set_partitions(torus_partitions *torus_p, int p_x, int p_y, int p_z);

// calculate the total torus nodes number
int get_nodes_num(struct torus_partitions t_p);

int translate_coordinates(torus_s *torus, int direction);

// assign torus node ip address
int assign_node_ip(torus_node *node_ptr);

int assign_cluster_id();

// return the index of torus node in torus node list
int get_node_index(torus_partitions torus_p, int x, int y, int z);

int set_neighbors(torus_s *torus, torus_node *node_ptr);

void assign_torus_leader(torus_s *torus);

// create a new torus
torus_s *new_torus(struct torus_partitions new_torus_p);

/* construct a newly created torus
 * construct steps:
 *      create a new torus 
 *      set global variable torus_node_list
 *      set neighbors for every torus node
 */
torus_s *create_torus(struct torus_partitions tp);

// append a extra new torus
torus_s *append_torus(torus_s *to, torus_s *from, int direction);


//int send_partition_info(const char *dst_ip, struct torus_partitions torus_p);

// dispatch all nodes and theirs neighbors info via socket
int dispatch_torus(torus_s *torus);

// traverse the torus from entry ip
int traverse_torus(const char *entry_ip);

void print_torus(torus_s *torus);

int traverse_skip_list(const char *entry_ip);

int dispatch_skip_list(skip_list *list, node_info leaders[]);

int query_torus(struct query_struct query, const char *entry_ip);

int query_oct_tree(struct query_struct query, const char *entry_ip);

// used for choose leaders
void shuffle(int array[], int n);

#endif /* CONTROL_H_ */

