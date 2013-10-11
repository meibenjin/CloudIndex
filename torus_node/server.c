/*
 * server.c
 *
 *  Created on: Sep 24, 2013
 *      Author: meibenjin
 */

#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include"server.h"
#include"logs/log.h"

__asm__(".symver memcpy,memcpy@GLIBC_2.2.5");

int init_request_list() {
	req_list = (struct request *) malloc(sizeof(struct request));
	if (req_list == NULL) {
		printf("malloc request list failed.\n");
		return FALSE;
	}
	req_list->first_run = TRUE;
	req_list->receive_num = 0;
	memset(req_list->stamp, 0, STAMP_SIZE);
	req_list->next = NULL;
	return TRUE;
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

int get_request(const char *req_stamp, struct request *req_ptr) {
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

	new_req->first_run = TRUE;
	new_req->receive_num = 0;
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

int gen_request_stamp(char *stamp) {
	if (stamp == NULL) {
		printf("gen_request_stamp: stamp is null pointer.\n");
		return FALSE;
	}
	// TODO automatic generate number stamp
	static long number_stamp = 1;
	char ip_stamp[IP_ADDR_LENGTH];
	if (FALSE == get_local_ip(ip_stamp)) {
		return FALSE;
	}
	snprintf(stamp, STAMP_SIZE, "%s_%ld", ip_stamp, number_stamp++);
	return TRUE;
}

int do_traverse_torus(int socketfd, struct message msg) {
	char stamp[STAMP_SIZE];
	memset(stamp, 0, STAMP_SIZE);

	int receive_num = get_neighbors_num(local_torus_node);
	if (strcmp(msg.stamp, "") == 0) {
		if (FALSE == gen_request_stamp(stamp)) {
			return FALSE;
		}
		memcpy(msg.stamp, stamp, STAMP_SIZE);
		receive_num += 1;
	} else {
		memcpy(stamp, msg.stamp, STAMP_SIZE);
	}

	if (FALSE == find_request(stamp)) {
		insert_request(stamp);
		printf("traverse torus: %s->%s\n", msg.src_ip, msg.dst_ip);

		//write log
		char buf[1024];
		memset(buf, 0, 1024);
		sprintf(buf, "traverse torus: %s->%s\n", msg.src_ip, msg.dst_ip);
		write_log(TORUS_NODE_LOG, buf);
	}

	/*struct reply_message reply_msg;
	 set_reply_message(&reply_msg, msg.op, msg.stamp, SUCCESS);
	 send_reply(socketfd, reply_msg);*/

	struct request *req_ptr = req_list->next;
	if (req_ptr && (TRUE == get_request(stamp, req_ptr))) {
		if (req_ptr->first_run == TRUE) {
			forward_to_neighbors(msg);
			req_ptr->first_run = FALSE;
			req_ptr->receive_num = receive_num;
		}
		req_ptr->receive_num--;
		if (req_ptr->receive_num == 0) {
			remove_request(stamp);
		}
	}
	return TRUE;
}

int forward_to_neighbors(struct message msg) {
	int i, forward_num = 0;
	int socketfd;
	char src_ip[IP_ADDR_LENGTH], dst_ip[IP_ADDR_LENGTH];
	int neighbors_num = get_neighbors_num(local_torus_node);

	for (i = 0; i < neighbors_num; ++i) {
		get_node_ip(*local_torus_node.neighbors[i], dst_ip);
		/*if (strcmp(dst_ip, msg.src_ip) == 0) {
		 continue;
		 }*/
		get_node_ip(local_torus_node, src_ip);

		strncpy(msg.src_ip, src_ip, IP_ADDR_LENGTH);
		strncpy(msg.dst_ip, dst_ip, IP_ADDR_LENGTH);

		socketfd = new_client_socket(dst_ip);
		if (FALSE == socketfd) {
			return FALSE;
		}

		if (TRUE == send_message(socketfd, msg)) {
			printf("\tforward message: %s -> %s\n", msg.src_ip, msg.dst_ip);

			//write log
			char buf[1024];
			memset(buf, 0, 1024);
			sprintf(buf, "forward message: %s -> %s\n", msg.src_ip, msg.dst_ip);
			write_log(TORUS_NODE_LOG, buf);

			/*if (TRUE == receive_reply(socketfd, &reply_msg)) {
			 if ((SUCCESS == reply_msg.reply_code)
			 && (strcmp(reply_msg.stamp, msg.stamp) == 0)) {
			 ;
			 }
			 }*/
		}
		close(socketfd);
	}
	return TRUE;
}

int do_update_torus(struct message msg) {
	int i;
	int nodes_num = 0;
	printf("update torus: %s -> %s\n", msg.src_ip, msg.dst_ip);

	memcpy(&nodes_num, msg.data, sizeof(int));
	if (nodes_num <= 0) {
		printf("update_torus_node: torus nodes number is wrong\n");
		return FALSE;
	}

	struct node_info nodes[nodes_num];

	for (i = 0; i < nodes_num; ++i) {
		memcpy(&nodes[i],
				(void*) (msg.data + sizeof(int) + sizeof(struct node_info) * i),
				sizeof(struct node_info));
	}

	// create torus node from several nodes
	if (FALSE == construct_torus_node(&local_torus_node, nodes)) {
		return FALSE;
	}
	return TRUE;
}

void set_reply_message(struct reply_message *reply_msg, const int op,
		const char *stamp, const int reply_code) {
	reply_msg->op = op;
	reply_msg->reply_code = reply_code;
	strncpy(reply_msg->stamp, stamp, STAMP_SIZE);
}

int process_message(int socketfd, struct message msg) {
	int reply_code = SUCCESS;
	switch (msg.op) {

	case UPDATE_TORUS:

		write_log(TORUS_NODE_LOG, "receive request update torus.\n");

		if (FALSE == do_update_torus(msg)) {
			reply_code = FAILED;
		}

		struct reply_message reply_msg;
		reply_msg.op = msg.op;
		reply_msg.reply_code = reply_code;
		strncpy(reply_msg.stamp, msg.stamp, STAMP_SIZE);

		if (FALSE == send_reply(socketfd, reply_msg)) {
			// TODO handle send reply failed
			return FALSE;
		}
		print_torus_node(local_torus_node);
		break;

	case TRAVERSE:
		do_traverse_torus(socketfd, msg);
		break;

	default:
		reply_code = WRONG_OP;

	}

	return TRUE;
}

int new_server() {
	int server_socket;
	server_socket = new_server_socket();
	if (server_socket == FALSE) {
		return FALSE;
	}
	return server_socket;
}

