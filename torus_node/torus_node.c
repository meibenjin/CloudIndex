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

void init_request_list() {
	strncpy(req_list->stamp, "", STAMP_SIZE);
	req_list->next = NULL;
}

int find_request(const char *req_stamp) {
	struct request *req_ptr = req_list->next;
	while (req_ptr) {
		if (strcmp(req_ptr->stamp, req_stamp) == 0) {
			return TRUE;
		}
		req_ptr = req_ptr->next;
	}
	return FALSE;
}

int insert_request(const char *req_stamp) {
	struct request *new_req = (struct request *) malloc(sizeof(struct request));
	if (new_req == NULL) {
		printf("insert_request: allocate new request failed.\n");
		return FALSE;
	}
	strncpy(new_req->stamp, req_stamp, STAMP_SIZE);
	new_req->next = NULL;

	struct request *req_ptr = req_list;
	new_req->next = req_ptr->next;
	req_ptr->next = new_req;

	return TRUE;
}

int remove_request(const char *req_stamp) {
	struct request *pre_ptr = req_list;
	struct request *req_ptr = req_list->next;
	while (req_ptr) {
		if (strcmp(req_ptr->stamp, req_stamp) == 0) {
			pre_ptr->next = req_ptr->next;
			free(req_ptr);
			req_ptr = NULL;
			return TRUE;
		}
		pre_ptr = pre_ptr->next;
		req_ptr = req_ptr->next;
	}
	return FALSE;
}

int traverse_torus_node(int socketfd, struct message msg) {
	char stamp[STAMP_SIZE];
	memset(stamp, 0, STAMP_SIZE);

	int neighbors_num = get_neighbors_num(local_torus_node);
	static int count = 0;
	if (strcmp(msg.stamp, "") == 0) {
		if (FALSE == gen_request_stamp(stamp)) {
			return FALSE;
		}
		memcpy(msg.stamp, stamp, STAMP_SIZE);

		insert_request(stamp);
		printf("traverse torus: %s->%s\n", msg.src_ip, msg.dst_ip);

	} else {
		count++;
		memcpy(stamp, msg.stamp, STAMP_SIZE);
		if (TRUE == find_request(stamp)) {
			;
		} else {
			insert_request(stamp);
			printf("traverse torus: %s->%s\n", msg.src_ip, msg.dst_ip);
		}
	}

	if (is_visited == FALSE) {
		forward_message(msg);
		is_visited = TRUE;
	}

	if (count == neighbors_num) {
		remove_request(stamp);
		is_visited = FALSE;
		count = 0;
	}

	return TRUE;
}

int forward_message(struct message msg) {
	int i, send_count;
	int socketfd;
	char src_ip[IP_ADDR_LENGTH], dst_ip[IP_ADDR_LENGTH];
	int neighbors_num = get_neighbors_num(local_torus_node);

	for (i = 0; i < neighbors_num; ++i) {
		get_node_ip(*local_torus_node.neighbors[i], dst_ip);
		/*if(strcmp(dst_ip, msg.src_ip) == 0){
		 continue;
		 }*/
		get_node_ip(local_torus_node, src_ip);

		strncpy(msg.src_ip, src_ip, IP_ADDR_LENGTH);
		strncpy(msg.dst_ip, dst_ip, IP_ADDR_LENGTH);

		socketfd = new_client_socket(dst_ip);
		if (FALSE == socketfd) {
			return FALSE;
		}

		send_message(socketfd, msg);
		close(socketfd);
	}
	return TRUE;
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
		if (TRUE == update_torus_node(msg))
			reply_code = SUCCESS;
		else
			reply_code = FAILED;

		if (FALSE == send_reply(socketfd, reply_code)) {
			return FALSE;
		}
		print_torus_node(local_torus_node);
		break;
	case TRAVERSE:
		traverse_torus_node(socketfd, msg);
		break;

	default:
		reply_code = WRONG_OP;

	}

	return TRUE;
}

