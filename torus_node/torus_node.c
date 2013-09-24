/*
 * torus_node.c
 *
 *  Created on: Sep 16, 2013
 *      Author: meibenjin
 */

#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include"torus_node.h"

void set_node_ip(torus_node *node_ptr, const char *ip) {
	if (!node_ptr) {
		printf("set_node_ip: node_ptr is null pointer.\n");
		return;
	}
	strncpy(node_ptr->info.ip, ip, IP_ADDR_LENGTH);
}

void get_node_ip(torus_node node, char *ip) {
	if (ip == NULL) {
		printf("get_node_ip: ip is null pointer.\n");
		return;
	}
	strncpy(ip, node.info.ip, IP_ADDR_LENGTH);
}

void set_coordinate(torus_node *node_ptr, int x, int y, int z) {
	if (!node_ptr) {
		printf("set_coordinate: node_ptr is null pointer.\n");
		return;
	}
	node_ptr->info.id.x = x;
	node_ptr->info.id.y = y;
	node_ptr->info.id.z = z;
}

struct coordinate get_coordinate(torus_node node){
    return node.info.id;
}

void set_neighbors_num(torus_node *node_ptr, int neighbors_num) {
	if (!node_ptr) {
		printf("set_neighbors_num: node_ptr is null pointer.\n");
		return;
	}

	node_ptr->info.neighbors_num = neighbors_num;
}

int get_neighbors_num(torus_node node) {
	return node.info.neighbors_num;
}

void print_neighbors(torus_node node) {
	int i;
	for (i = 0; i < node.info.neighbors_num; ++i) {
		if (node.neighbors[i]) {
			printf("\t");
			print_node_info(node.neighbors[i]->info);
		}
	}
}

void init_torus_node(torus_node *node_ptr) {
	int i;
	set_node_ip(node_ptr, "0.0.0.0");

	set_coordinate(node_ptr, -1, -1, -1);

	set_neighbors_num(node_ptr, 0);

	for (i = 0; i < MAX_NEIGHBORS; ++i) {
		node_ptr->neighbors[i] = NULL;
	}
}

int create_torus_node(torus_node *torus, node_info *nodes) {
	if (torus == NULL) {
		printf("create_torus_node: torus is null pointer.\n");
		return FALSE;
	}
	if (nodes == NULL) {
		printf("create_torus_node: nodes info is null pointer.\n");
		return FALSE;
	}
	torus->info = nodes[0];
	int neighbors_num = nodes[0].neighbors_num;
	int i;
	for (i = 0; i < neighbors_num; ++i) {
		torus_node *new_node = (torus_node *) malloc(sizeof(torus_node));

		init_torus_node(new_node);
		new_node->info = nodes[i + 1];
		torus->neighbors[i] = new_node;
	}
	return TRUE;
}

void print_torus_node(torus_node torus) {
	print_node_info(torus.info);
	print_neighbors(torus);
	printf("\n");
}

void print_node_info(node_info node) {
	printf("(%d, %d, %d)\t%s\n", node.id.x, node.id.y, node.id.z, node.ip);
}



