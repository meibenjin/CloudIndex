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
#include"skip_list/skip_list.h"

//__asm__(".symver memcpy,memcpy@GLIBC_2.2.5");

void init_node_info(node_info *info_ptr) {
	if (!info_ptr) {
		printf("init_torus_node: node_ptr is null pointer.\n");
		return;
	}
	set_node_ip(info_ptr, "0.0.0.0");
	set_node_id(info_ptr, -1, -1, -1);
	set_cluster_id(info_ptr, -1);
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

void print_neighbors(torus_node node) {
	int i;
	int neighbors_num = get_neighbors_num(node);
	for (i = 0; i < neighbors_num; ++i) {
		if (node.neighbors[i] != NULL) {
			printf("\t");
			print_node_info(node.neighbors[i]->info);
		}
	}
}

void init_torus_node(torus_node *node_ptr) {
	if (!node_ptr) {
		printf("init_torus_node: node_ptr is null pointer.\n");
		return;
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
	print_node_info(torus.info);
	print_neighbors(torus);
	printf("\n");
}

void print_node_info(node_info node) {
	coordinate c = node.node_id;
	printf("%d\t(%d, %d, %d)\t%s\n", node.cluster_id, c.x, c.y, c.z, node.ip);

	char buf[1024];
	memset(buf, 0, 1024);
	sprintf(buf, "%d\t(%d, %d, %d)\t%s\n", node.cluster_id, c.x, c.y, c.z,
			node.ip);
	write_log(TORUS_NODE_LOG, buf);
}

