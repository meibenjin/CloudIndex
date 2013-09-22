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

int traverse_torus_node(struct message msg) {
    char stamp[STAMP_SIZE];
    memset(stamp, 0, STAMP_SIZE);
    if(strcmp(msg.data, "") == 0) {
        if(FALSE == gen_request_stamp(stamp)){
            return FALSE;
        }
    } else {
        strncpy(stamp, msg.data, STAMP_SIZE);
    }
    request_list(stamp);
    forward_message(msg.src_ip, msg);
}

int update_torus_node(struct message msg) {
	int i;
	int nodes_num = 0;

	memcpy(&nodes_num, msg.data, sizeof(int));
	if (nodes_num <= 0) {
		printf("do_update_torus: torus nodes num is wrong\n");
		return FALSE;
	}

	struct node_info nodes[nodes_num];

	for (i = 0; i < nodes_num; ++i) {
		memcpy(&nodes[i],
				(void*) (msg.data + sizeof(int) + sizeof(struct node_info) * i),
				sizeof(struct node_info));
	}

	// create torus node from serval nodes
	if (FALSE == create_torus_node(&local_torus_node, nodes)) {
		return FALSE;
	}
	return TRUE;
}

int process_message(int socketfd, struct message msg) {
	int reply_code;
	switch (msg.op) {

	case UPDATE_TORUS:
		printf("request: update torus.\n");
        printf("request stamp: %s\n", msg.req_stamp);
		if (TRUE == update_torus_node(msg))
			reply_code = SUCCESS;
		else
			reply_code = FAILED;
		break;

    case TRAVERSE:
        printf("request: traverse torus.\n");
		if (TRUE == traverse_torus_node(msg))
			reply_code = SUCCESS;
		else
			reply_code = FAILED;
		break;

	default:
		reply_code = WRONG_OP;

	}

	if (FALSE == send_reply(socketfd, reply_code)) {
		return FALSE;
	}
	print_torus_node(local_torus_node);
	return TRUE;
}

