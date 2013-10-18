/*
 * skip_list.h
 *
 *  Created on: Oct 2, 2013
 *      Author: meibenjin
 */

#ifndef SKIP_LIST_H_
#define SKIP_LIST_H_

#include"utils.h"
#include"torus_node/torus_node.h"

#define MAXLEVEL 31

//skip list node structure
typedef struct skip_list_node {
	node_info *leader;
    int height;
    struct skip_list_level{
        struct skip_list_node *forward;
        struct skip_list_node *backward;
    }level[];
}skip_list_node;

// header of skip_list
typedef struct skip_list{
	skip_list_node *header;
	int level;
}skip_list;

// compare two torus_node by their torus cluster id
int compare(node_info *node1, node_info *node2);

// random choose the skip list level
int random_level();

// create skip_list list
skip_list *new_skip_list();

// create a skip list node from a torus node
skip_list_node *new_skip_list_node(int level, node_info *node_ptr);

// insert a torus_node into skip_list
int insert_skip_list(skip_list *slist, node_info *node_ptr);

// remove a torus_node from skip_list
int remove_skip_list(skip_list *slist, node_info *node_ptr);

// find out a torus_node from skip_list
int search_skip_list(skip_list *slist, node_info *node_ptr);

// traverse the skip_list
void traverse_skip_list(skip_list *slist);

void print_skip_list_node(skip_list_node *sln_ptr);

skip_list_node *construct_skip_list_node_info(int nodes_num, skip_list_node *node_ptr);

#endif /* SKIP_LIST_H_ */



