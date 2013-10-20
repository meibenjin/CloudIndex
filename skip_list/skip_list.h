/*
 * skip_list.h
 *
 *  Created on: Oct 2, 2013
 *      Author: meibenjin
 */

#ifndef SKIP_LIST_H_
#define SKIP_LIST_H_

#include"utils.h"


// compare two torus_node by their torus cluster id
int compare(node_info *node1, node_info *node2);

// random choose the skip list level
int random_level();

// create skip_list list
skip_list *new_skip_list(int level);

// create a skip list node from a torus node
skip_list_node *new_skip_list_node(int level, node_info *node_ptr);

// insert a torus_node into skip_list
int insert_skip_list(skip_list *slist, node_info *node_ptr);

// remove a torus_node from skip_list
int remove_skip_list(skip_list *slist, node_info *node_ptr);

// find out a torus_node from skip_list
int search_skip_list(skip_list *slist, node_info *node_ptr);

// traverse the skip_list
void print_skip_list(skip_list *slist);

void print_skip_list_node(skip_list *slist);

#endif /* SKIP_LIST_H_ */



