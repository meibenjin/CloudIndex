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

#define MAXLEVEL 32 

//skip list node structure
typedef struct skip_list_node {
	torus_node leader;
	struct skip_list_node *forward[1];
}skip_list_node;

// header of skip_list
typedef struct skip_list{
	skip_list_node *header;
	int level;
}skip_list;

// compare two torus_node by their torus cluster id
int compare(torus_node node1, torus_node node2);

// random determine the skip list level
int random_level();

// initialize skip_list list
int init_skip_list();

// insert a torus_node into skip_list
int insert_skip_list(torus_node node);

// remove a torus_node from skip_list
int remove_skip_list(torus_node node);

// find out a torus_node from skip_list
int search_skip_list(torus_node node);

// traverse the skip_list
void traverse_skip_list();

#endif /* SKIP_LIST_H_ */
