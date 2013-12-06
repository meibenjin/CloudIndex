#ifndef CONTROL_H_
#define CONTROL_H_

/*
 * control.h
 *
 *  Created on: Sep 20, 2013
 *      Author: meibenjin
 */

#include"utils.h"
#include"torus_node/torus_node.h"
#include"socket/socket.h"
#include"skip_list/skip_list.h"

#define TORUS_IP_LIST "../etc/torus_ip_list"

torus_cluster *cluster_list;

// skip list (multiple level linked list for torus cluster)
skip_list *slist;

char torus_ip_list[MAX_NODES_NUM][IP_ADDR_LENGTH];
int torus_nodes_num;

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

node_info *assign_torus_leader(torus_s *torus);

// create a new torus
torus_s *new_torus(struct torus_partitions new_torus_p);

/* construct a newly created torus
 * construct steps:
 *      create a new torus 
 *      set global variable torus_node_list
 *      set neighbors for every torus node
 */
torus_s *create_torus(int p_x, int p_y, int p_z);

// append a extra new torus
torus_s *append_torus(torus_s *to, torus_s *from, int direction);

// send nodes info to dst_ip
int send_nodes_info(OP op, const char *dst_ip, int nodes_num, struct node_info *nodes);

int send_partition_info(const char *dst_ip, struct torus_partitions torus_p);

// update all nodes and theirs neighbors info via socket
int update_torus(torus_s *torus);

// traverse the torus from entry ip
int traverse_torus(const char *entry_ip);

void print_torus(torus_s *torus);

int read_torus_ip_list();

// create a skip list
int create_skip_list(torus_s *torus);

int update_skip_list(skip_list *list);

int traverse_skip_list(const char *entry_ip);

int insert_skip_list_node(skip_list *list, node_info *node_ptr);

int search_skip_list_node(interval interval[], const char *entry_ip);

#endif /* CONTROL_H_ */

