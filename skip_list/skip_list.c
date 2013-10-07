/*
 * skip_list.c
 *
 *  Created on: Oct 2, 2013
 *      Author: meibenjin
 */

#include"skip_list.h"

#include<stdlib.h>
#include<stdio.h>

skip_list list;

//#define NIL list.header

int compare(torus_node node1, torus_node node2) {
	int id1 = node1.info.cluster_id;
	int id2 = node2.info.cluster_id;
	if (id1 < id2) {
		return -1;
	} else if (id1 == id2) {
		return 0;
	} else {
		return 1;
	}
}

int random_level() {
	int level = 0;
	while ((level < MAXLEVEL) && (rand() < RAND_MAX / 2)) {
		level++;
	}
	return level;
}

int init_skip_list() {
	int i;
	list.header = (skip_list_node *) malloc(
			sizeof(skip_list_node) + MAXLEVEL * sizeof(skip_list_node *));
	if (list.header == NULL) {
		printf("init_list:insufficient memory.\n");
		return FALSE;
	}
	for (i = 0; i < MAXLEVEL; ++i) {
		list.header->forward[i] = NULL;
	}
	list.level = 0;
	return TRUE;
}

int insert_skip_list(torus_node node) {
	int i, new_level;
	skip_list_node *update[MAXLEVEL + 1];
	skip_list_node *sln_ptr;
	sln_ptr = list.header;

	// find the position to insert the torus node
	for (i = list.level; i >= 0; --i) {
		while ((sln_ptr->forward[i] != NULL)
				&& (-1 == compare(sln_ptr->forward[i]->leader, node))) {
			sln_ptr = sln_ptr->forward[i];
		}
		update[i] = sln_ptr;
	}
	sln_ptr = sln_ptr->forward[0];
	if ((sln_ptr != NULL) && (0 == compare(sln_ptr->leader, node))) {
		printf("insert: duplicate torus node.\n");
		return FALSE;
	}

	// determine level
	new_level = random_level();
	if (new_level > list.level) {
		for (i = list.level + 1; i <= new_level; i++) {
			update[i] = list.header;
		}
		list.level = new_level;
	}
	sln_ptr = (skip_list_node *) malloc(
			sizeof(skip_list_node) + new_level * sizeof(skip_list_node *));
	if (sln_ptr == NULL) {
		printf("insert: allocate failed.\n");
		return FALSE;
	}

	sln_ptr->leader = node;
	// update forward links
	for (i = 0; i <= new_level; ++i) {
		sln_ptr->forward[i] = update[i]->forward[i];
		update[i]->forward[i] = sln_ptr;
	}
	return TRUE;
}

int remove_skip_list(torus_node node) {
	int i, new_level;
	skip_list_node *update[MAXLEVEL + 1];
	skip_list_node *sln_ptr;
	sln_ptr = list.header;

	// find the position to insert the torus node
	for (i = list.level; i >= 0; --i) {
		while ((sln_ptr->forward[i] != NULL)
				&& (-1 == compare(sln_ptr->forward[i]->leader, node))) {
			sln_ptr = sln_ptr->forward[i];
		}
		update[i] = sln_ptr;
	}

	sln_ptr = sln_ptr->forward[0];
	if ((sln_ptr == NULL) || (0 != compare(sln_ptr->leader, node))) {
		printf("remove: can't find torus node.\n");
		return FALSE;
	}

	/* adjust forward pointers */
	for (i = 0; i <= list.level; i++) {
		if (update[i]->forward[i] != sln_ptr)
			break;
		update[i]->forward[i] = sln_ptr->forward[i];
	}
	free(sln_ptr);

	/* adjust header level */
	while ((list.level > 0) && (list.header->forward[list.level] == NULL))
		list.level--;

	return TRUE;
}

int search_skip_list(torus_node node) {
	int i;
	skip_list_node *sln_ptr;
	sln_ptr = list.header;

	// find the torus node position
	for (i = list.level; i >= 0; --i) {
		while ((sln_ptr->forward[i] != NULL)
				&& (-1 == compare(sln_ptr->forward[i]->leader, node))) {
			sln_ptr = sln_ptr->forward[i];
		}
	}

	sln_ptr = sln_ptr->forward[0];
	if ((sln_ptr != NULL) && (0 == compare(sln_ptr->leader, node))) {
		print_node_info(sln_ptr->leader);
		return TRUE;
	}
	return FALSE;
}

void traverse_skip_list() {
	int i;
	skip_list_node *sln_ptr;
	sln_ptr = list.header;
	for (i = 0; i <= list.level; ++i) {
		sln_ptr = list.header;
		printf("level:%d\n", i);
		while (sln_ptr->forward[i] != NULL) {
			print_node_info(sln_ptr->forward[i]->leader);
			sln_ptr = sln_ptr->forward[i];
		}
		printf("\n");
	}

}

