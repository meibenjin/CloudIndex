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
#include"skip_list/skip_list.h"

__asm__(".symver memcpy,memcpy@GLIBC_2.2.5");

request *new_request() {
	request *req_ptr;
	req_ptr = (struct request *) malloc(sizeof(request));
	if (req_ptr == NULL) {
		printf("malloc request list failed.\n");
		return NULL;
	}
	req_ptr->first_run = TRUE;
	req_ptr->receive_num = 0;
	memset(req_ptr->stamp, 0, STAMP_SIZE);
	req_ptr->next = NULL;
	return req_ptr;
}

request *find_request(request *list, const char *req_stamp) {
	request *req_ptr = list->next;
	while (req_ptr) {
		if (strcmp(req_ptr->stamp, req_stamp) == 0) {
			return req_ptr;
		}
		req_ptr = req_ptr->next;
	}
	return NULL;
}

request *insert_request(request *list, const char *req_stamp) {
	struct request *new_req = new_request();
	if (new_req == NULL) {
		printf("insert_request: allocate new request failed.\n");
		return NULL;
	}
	strncpy(new_req->stamp, req_stamp, STAMP_SIZE);

	request *req_ptr = list;
	new_req->next = req_ptr->next;
	req_ptr->next = new_req;

	return new_req;
}

int remove_request(request *list, const char *req_stamp) {
	struct request *pre_ptr = list;
	struct request *req_ptr = list->next;
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

	int neighbors_num = get_neighbors_num(the_torus_node);

	if (strcmp(msg.stamp, "") == 0) {
		if (FALSE == gen_request_stamp(stamp)) {
			return FALSE;
		}
		strncpy(msg.stamp, stamp, STAMP_SIZE);
		neighbors_num += 1;
	} else {
		strncpy(stamp, msg.stamp, STAMP_SIZE);
	}

	request *req_ptr = find_request(req_list, stamp);

	if (NULL == req_ptr) {
		req_ptr = insert_request(req_list, stamp);
		printf("traverse torus: %s->%s\n", msg.src_ip, msg.dst_ip);

		//write log
		char buf[1024];
		memset(buf, 0, 1024);
		sprintf(buf, "traverse torus: %s->%s\n", msg.src_ip, msg.dst_ip);
		write_log(TORUS_NODE_LOG, buf);

		if (req_ptr && (req_ptr->first_run == TRUE)) {
			forward_to_neighbors(msg);
			req_ptr->first_run = FALSE;
			req_ptr->receive_num = neighbors_num;
		}

	} else {
		req_ptr->receive_num--;
		if (req_ptr->receive_num == 0) {
			remove_request(req_list, stamp);
		}
	}

	/*struct reply_message reply_msg;
	 set_reply_message(&reply_msg, msg.op, msg.stamp, SUCCESS);
	 send_reply(socketfd, reply_msg);*/

	return TRUE;
}

int forward_to_neighbors(struct message msg) {
	int i, forward_num = 0;
	int socketfd;
	char src_ip[IP_ADDR_LENGTH], dst_ip[IP_ADDR_LENGTH];

	int neighbors_num = get_neighbors_num(the_torus_node);

    get_node_ip(the_torus_node.info, src_ip);
	for (i = 0; i < neighbors_num; ++i) {
		get_node_ip(the_torus_node.neighbors[i]->info, dst_ip);
		/*if (strcmp(dst_ip, msg.src_ip) == 0) {
		 continue;
		 }*/

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

    //write log
	char buf[1024];
	memset(buf, 0, 1024);
	sprintf(buf, "update torus: %s -> %s\n", msg.src_ip, msg.dst_ip);
	write_log(TORUS_NODE_LOG, buf);

	memcpy(&nodes_num, msg.data, sizeof(int));
	if (nodes_num <= 0) {
		printf("update_torus_node: torus nodes number is wrong\n");
		return FALSE;
	}

	node_info nodes[nodes_num];

	for (i = 0; i < nodes_num; ++i) {
		memcpy(&nodes[i],
				(void*) (msg.data + sizeof(int) + sizeof(node_info) * i),
				sizeof(node_info));
	}

	// construct a torus node from several nodes
	the_torus_node.info = nodes[0];
	set_neighbors_num(&the_torus_node, nodes_num - 1);
	for (i = 1; i < nodes_num; ++i) {
		torus_node *new_node = new_torus_node();
		if (new_node == NULL) {
			return FALSE;
		}
		new_node->info = nodes[i];
		the_torus_node.neighbors[i - 1] = new_node;
	}
	return TRUE;
}

int do_update_skip_list(struct message msg) {
	int i;
	int nodes_num = 0;
	printf("update skip_list: %s -> %s\n", msg.src_ip, msg.dst_ip);
    
    //write log
	char buf[1024];
	memset(buf, 0, 1024);
	sprintf(buf, "update skip_list: %s -> %s\n", msg.src_ip, msg.dst_ip);
	write_log(TORUS_NODE_LOG, buf);

	memcpy(&nodes_num, msg.data, sizeof(int));
	if (nodes_num <= 0) {
		printf("do_update_skip_list: skip_list node number is wrong\n");
		return FALSE;
	}

	node_info nodes[nodes_num]; //= (skip_list_node_info *) malloc(sizeof(skip_list_node_info) *nodes_num);

	for (i = 0; i < nodes_num; ++i) {
		memcpy(&nodes[i],(void*) (msg.data + sizeof(int) + sizeof(node_info) * i),sizeof(node_info));
	}

	memset(buf, 0, 1024);
	sprintf(buf, "receive message: %d\n", nodes_num);
	write_log(TORUS_NODE_LOG, buf);
    // create torus node from several nodes
    //local_skip_list_node_info = construct_skip_list_node_info(nodes_num, nodes);
	/*if (local_skip_list_node_info == NULL) {
	 return FALSE;
	 }*/
	return TRUE;
}

int process_message(int socketfd, struct message msg) {

	struct reply_message reply_msg;
	int reply_code = SUCCESS;
	switch (msg.op) {

	case UPDATE_TORUS:

		write_log(TORUS_NODE_LOG, "receive request update torus.\n");

		if (FALSE == do_update_torus(msg)) {
			reply_code = FAILED;
		}

		reply_msg.op = msg.op;
		reply_msg.reply_code = reply_code;
		strncpy(reply_msg.stamp, msg.stamp, STAMP_SIZE);

		if (FALSE == send_reply(socketfd, reply_msg)) {
			// TODO handle send reply failed
			return FALSE;
		}
		print_torus_node(the_torus_node);
		break;
	case UPDATE_SKIP_LIST:
		write_log(TORUS_NODE_LOG, "receive request update skip list.\n");
		if (FALSE == do_update_skip_list(msg)) {
			reply_code = FAILED;
		}
		reply_msg.op = msg.op;
		reply_msg.reply_code = reply_code;
		strncpy(reply_msg.stamp, msg.stamp, STAMP_SIZE);
		if (FALSE == send_reply(socketfd, reply_msg)) {
			// TODO handle send reply failed
			return FALSE;
		}
		//print_skip_list_node(local_sl_node);
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

