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
#include"logs/log.h"

//__asm__(".symver memcpy,memcpy@GLIBC_2.2.5");

void init_node_info(node_info *info_ptr) {
	if (!info_ptr) {
		printf("init_torus_node: node_ptr is null pointer.\n");
		return;
	}
	set_node_ip(info_ptr, "0.0.0.0");
	set_node_id(info_ptr, -1, -1, -1);
	set_cluster_id(info_ptr, -1);

	int i;
	for(i = 0; i < MAX_DIM_NUM; ++i){
		info_ptr->dims[i].low = 0;
		info_ptr->dims[i].high = 0;
	}

}

void init_torus_node(torus_node *node_ptr) {
	if (!node_ptr) {
		printf("init_torus_node: node_ptr is null pointer.\n");
		return;
	}
    init_node_info(&node_ptr->info);
}

int assign_dimensions(node_info *node_ptr){
	static int last_dim[MAX_DIM_NUM] = {0, 0, 0};
	int i;
	for(i = 0; i < MAX_DIM_NUM; ++i){
		node_ptr->dims[i].low = last_dim[i] + rand() % 10;
		node_ptr->dims[i].high = node_ptr->dims[i].low + rand() % 10;
		last_dim[i] = node_ptr->dims[i].high;
	}

	return TRUE;
}

void set_node_ip(node_info *info_ptr, const char *ip) {
	if (!info_ptr) {
		printf("set_node_ip: node_ptr is null pointer.\n");
		return;
	}
	strncpy(info_ptr->ip, ip, IP_ADDR_LENGTH);
}

void get_node_ip(node_info info, char *ip) {
	if (ip == NULL) {
		printf("get_node_ip: ip is null pointer.\n");
		return;
	}
	strncpy(ip, info.ip, IP_ADDR_LENGTH);
}

void set_node_id(node_info *info_ptr, int x, int y, int z) {
	if (!info_ptr) {
		printf("set_coordinate: node_ptr is null pointer.\n");
		return;
	}
	info_ptr->node_id.x = x;
	info_ptr->node_id.y = y;
	info_ptr->node_id.z = z;
}

struct coordinate get_node_id(node_info info) {
	return info.node_id;
}

void set_cluster_id(node_info *info_ptr, int cluster_id) {
	if (!info_ptr) {
		printf("set_cluster_id: node_ptr is null pointer.\n");
		return;
	}
	info_ptr->cluster_id = cluster_id;
}

int get_cluster_id(node_info info) {
	return info.cluster_id;
}

void set_neighbors_num(torus_node *node_ptr, int neighbors_num) {
	if (!node_ptr) {
		printf("set_neighbors_num: node_ptr is null pointer.\n");
		return;
	}

	node_ptr->neighbors_num = neighbors_num;
}

int get_neighbors_num(torus_node node) {
	return node.neighbors_num;
}

node_info *get_neighbor_by_id(torus_node node, struct coordinate node_id){
	int i, x, y, z;
	int neighbors_num = get_neighbors_num(node);
	for (i = 0; i < neighbors_num; ++i) {
        x = node.neighbors[i]->info.node_id.x;
        y =  node.neighbors[i]->info.node_id.y; 
        z =  node.neighbors[i]->info.node_id.z; 
		if ((node_id.x == x) && (node_id.y == y) && (node_id.z == z)) {
            return &node.neighbors[i]->info;
		}
	}
    return NULL;
}

void print_neighbors(torus_node node) {
	int i;
	int neighbors_num = get_neighbors_num(node);
	for (i = 0; i < neighbors_num; ++i) {
		if (node.neighbors[i] != NULL) {
			printf("\tneighbors:");
            write_log(TORUS_NODE_LOG, "\tneighbor:");

			print_node_info(node.neighbors[i]->info);
		}
	}
}

torus_node *new_torus_node() {
	torus_node *new_torus;
	new_torus = (torus_node *) malloc(sizeof(torus_node));
	if (new_torus == NULL) {
		printf("new_torus_node: malloc for torus node failed.\n");
		return NULL;
	}
	init_node_info(&new_torus->info);
	set_neighbors_num(new_torus, 0);
	int i;
	for (i = 0; i < MAX_NEIGHBORS; ++i) {
		new_torus->neighbors[i] = NULL;
	}
	return new_torus;
}

void print_torus_node(torus_node torus) {
    write_log(TORUS_NODE_LOG, "myself:");
	print_node_info(torus.info);
	print_neighbors(torus);
	printf("\n");
}

void print_node_info(node_info node) {
	int i;

	printf("%s:", node.ip);
	for(i = 0; i < MAX_DIM_NUM; ++i){
		printf("[%.15f, %.15f] ", node.dims[i].low, node.dims[i].high);
	}
	printf("\n");

	char buf[1024];
	memset(buf, 0, 1024);
	int len = sprintf(buf, "%s:", node.ip);
	for(i = 0; i < MAX_DIM_NUM; ++i){
			len += sprintf(buf + len, "[%.15f, %15f] ", node.dims[i].low, node.dims[i].high);
	}
	sprintf(buf+len, "\n");

	write_log(TORUS_NODE_LOG, buf);
}

