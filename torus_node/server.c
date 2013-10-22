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

int do_traverse_torus(struct message msg) {
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
		printf("traverse torus: %s -> %s\n", msg.src_ip, msg.dst_ip);

		//write log
		char buf[1024];
		memset(buf, 0, 1024);
		sprintf(buf, "traverse torus: %s -> %s\n", msg.src_ip, msg.dst_ip);
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
	char src_ip[IP_ADDR_LENGTH], dst_ip[IP_ADDR_LENGTH];

	int neighbors_num = get_neighbors_num(the_torus_node);

	get_node_ip(the_torus_node.info, src_ip);
	for (i = 0; i < neighbors_num; ++i) {
		get_node_ip(the_torus_node.neighbors[i]->info, dst_ip);

		strncpy(msg.src_ip, src_ip, IP_ADDR_LENGTH);
		strncpy(msg.dst_ip, dst_ip, IP_ADDR_LENGTH);
		//forward_message(msg);
        int socketfd;
        socketfd = new_client_socket(msg.dst_ip);
        if (FALSE == socketfd) {
            return FALSE;
        }

        if (TRUE == send_message(socketfd, msg)) {
            printf("\tforward message: %s -> %s\n", msg.src_ip, msg.dst_ip);

            //write log
            char buf[1024];
            memset(buf, 0, 1024);
            sprintf(buf, "\tforward message: %s -> %s\n", msg.src_ip, msg.dst_ip);
            write_log(TORUS_NODE_LOG, buf);

        }
        close(socketfd);
	}
	return TRUE;
}

int do_update_torus(struct message msg) {
	int i;
	int nodes_num = 0;

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

	memcpy(&nodes_num, msg.data, sizeof(int));
	if (nodes_num <= 0) {
		printf("do_update_skip_list: skip_list node number is wrong\n");
		return FALSE;
	}

	node_info nodes[nodes_num];

	for (i = 0; i < nodes_num; ++i) {
		memcpy(&nodes[i],
				(void*) (msg.data + sizeof(int) + sizeof(node_info) * i),
				sizeof(node_info));
	}

	int index = 0;
	int level = (nodes_num / 2) - 1;
	skip_list_node *sln_ptr, *new_sln;
    /*char buf[1024];
    memset(buf, 0, 1024);
    sprintf(buf, "update_skip_list:%d\n", nodes_num);
    write_log(TORUS_NODE_LOG, buf);

    for(i = 0; i < nodes_num; i++) {
        print_node_info(nodes[i]);
    }*/

	for (i = 0; i <= level; i++) {
		if (get_cluster_id(nodes[index]) != -1) {
			new_sln = new_skip_list_node(0, &nodes[index]);
            // free old forward field first
            if(sln_ptr->level[i].forward) {
                free(sln_ptr->level[i].forward);
            }
			sln_ptr->level[i].forward = new_sln;
			index++;
		} else {
			index++;
		}
		if (get_cluster_id(nodes[index]) != -1) {
			new_sln = new_skip_list_node(0, &nodes[index]);
            // free old forward field first
            if(sln_ptr->level[i].backward) {
                free(sln_ptr->level[i].backward);
            }
			sln_ptr->level[i].backward = new_sln;
			index++;
		} else {
			index++;
		}
	}

	return TRUE;
}

int do_traverse_skip_list(struct message msg) {
	char buf[1024];
	char src_ip[IP_ADDR_LENGTH], dst_ip[IP_ADDR_LENGTH];
	skip_list_node *cur_sln, *forward, *backward;
	cur_sln = the_skip_list.header;
	forward = cur_sln->level[0].forward;
	backward = cur_sln->level[0].backward;

	write_log(TORUS_NODE_LOG, "visit myself:");
	print_node_info(cur_sln->leader);

	get_node_ip(cur_sln->leader, src_ip);
	strncpy(msg.src_ip, src_ip, IP_ADDR_LENGTH);
	if (strcmp(msg.data, "") == 0) {

		if (forward) {
			get_node_ip(forward->leader, dst_ip);
			strncpy(msg.dst_ip, dst_ip, IP_ADDR_LENGTH);
			strncpy(msg.data, "forward", DATA_SIZE);
			forward_message(msg);
		}

		if (backward) {
			get_node_ip(backward->leader, dst_ip);
			strncpy(msg.dst_ip, dst_ip, IP_ADDR_LENGTH);
			strncpy(msg.data, "backward", DATA_SIZE);
			forward_message(msg);
		}

	} else if (strcmp(msg.data, "forward") == 0) {
		if (forward) {
			get_node_ip(forward->leader, dst_ip);
			strncpy(msg.dst_ip, dst_ip, IP_ADDR_LENGTH);
			forward_message(msg);
		}

	} else {
		if (backward) {
			get_node_ip(backward->leader, dst_ip);
			strncpy(msg.dst_ip, dst_ip, IP_ADDR_LENGTH);
			forward_message(msg);
		}
	}

	return TRUE;
}

int do_update_skip_list_node(struct message msg) {
	int i;
	// analysis node info from message
	memcpy(&i, msg.data, sizeof(int));

	node_info f_node, b_node;
	memcpy(&f_node, (void*) (msg.data + sizeof(int)), sizeof(node_info));
	memcpy(&b_node, (void*) (msg.data + sizeof(int) + sizeof(node_info)), sizeof(node_info));

	skip_list_node *sln_ptr, *new_sln;
	sln_ptr = the_skip_list.header;
	if (get_cluster_id(f_node) != -1) {
		new_sln = new_skip_list_node(0, &f_node);

        // free old forward field first
        if(sln_ptr->level[i].forward) {
            free(sln_ptr->level[i].forward);
        }

		sln_ptr->level[i].forward = new_sln;
	}

	if (get_cluster_id(b_node) != -1) {
		new_sln = new_skip_list_node(0, &b_node);

        // free old backward field first
        if(sln_ptr->level[i].backward) {
            free(sln_ptr->level[i].backward);
        }
		sln_ptr->level[i].backward = new_sln;
	}
    return TRUE;
}
int do_update_forward(struct message msg) {
	int i;
	// analysis node info from message
	memcpy(&i, msg.data, sizeof(int));

	node_info f_node;
	memcpy(&f_node, (void*) (msg.data + sizeof(int)), sizeof(node_info));

	skip_list_node *sln_ptr, *new_sln;
	sln_ptr = the_skip_list.header;
	if (get_cluster_id(f_node) != -1) {
		new_sln = new_skip_list_node(0, &f_node);

        // free old forward field first
        if(sln_ptr->level[i].forward) {
            free(sln_ptr->level[i].forward);
        }

		sln_ptr->level[i].forward = new_sln;
	}
    return TRUE;
}

int do_update_backward(struct message msg) {
	int i;
	// analysis node info from message
	memcpy(&i, msg.data, sizeof(int));

	node_info b_node;
	memcpy(&b_node, (void*) (msg.data + sizeof(int)), sizeof(node_info));

	skip_list_node *sln_ptr, *new_sln;
	sln_ptr = the_skip_list.header;
	if (get_cluster_id(b_node) != -1) {
		new_sln = new_skip_list_node(0, &b_node);

        // free old backward field first
        if(sln_ptr->level[i].backward) {
            free(sln_ptr->level[i].backward);
        }

		sln_ptr->level[i].backward= new_sln;
	}

}

int do_new_skip_list(struct message msg) {
	int level;
	skip_list_node *sln_ptr;
	// analysis node info from message
	memcpy(&level, msg.data, sizeof(int));

	the_skip_list = *new_skip_list(level);
	sln_ptr = the_skip_list.header;
	sln_ptr->leader = the_torus_node.info;
	sln_ptr->height = level;
	the_skip_list.level = level;
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

	case TRAVERSE_TORUS:
		write_log(TORUS_NODE_LOG, "receive request traverse torus.\n");
		do_traverse_torus(msg);
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
		print_skip_list_node(&the_skip_list);
		break;

	case UPDATE_SKIP_LIST_NODE:
		write_log(TORUS_NODE_LOG, "receive request update skip list node.\n");

		if (FALSE == do_update_skip_list_node(msg)) {
			reply_code = FAILED;
		}
		reply_msg.op = msg.op;
		reply_msg.reply_code = reply_code;
		strncpy(reply_msg.stamp, msg.stamp, STAMP_SIZE);
		if (FALSE == send_reply(socketfd, reply_msg)) {
			// TODO handle send reply failed
			return FALSE;
		}
		print_skip_list_node(&the_skip_list);
		break;
    
    case UPDATE_FORWARD:
		write_log(TORUS_NODE_LOG, "receive request update skip list node's forward field.\n");
		if (FALSE == do_update_forward(msg)) {
			reply_code = FAILED;
		}
		reply_msg.op = msg.op;
		reply_msg.reply_code = reply_code;
		strncpy(reply_msg.stamp, msg.stamp, STAMP_SIZE);
		if (FALSE == send_reply(socketfd, reply_msg)) {
			// TODO handle send reply failed
			return FALSE;
		}
		print_skip_list_node(&the_skip_list);
        break;

    case UPDATE_BACKWARD:
		write_log(TORUS_NODE_LOG, "receive request update skip list node's backward field.\n");

		if (FALSE == do_update_backward(msg)) {
			reply_code = FAILED;
		}
		reply_msg.op = msg.op;
		reply_msg.reply_code = reply_code;
		strncpy(reply_msg.stamp, msg.stamp, STAMP_SIZE);
		if (FALSE == send_reply(socketfd, reply_msg)) {
			// TODO handle send reply failed
			return FALSE;
		}
		print_skip_list_node(&the_skip_list);
		break;

	case NEW_SKIP_LIST:
		write_log(TORUS_NODE_LOG, "receive request new skip list.\n");
		if (FALSE == do_new_skip_list(msg)) {
			reply_code = FAILED;
		}
		reply_msg.op = msg.op;
		reply_msg.reply_code = reply_code;
		strncpy(reply_msg.stamp, msg.stamp, STAMP_SIZE);
		if (FALSE == send_reply(socketfd, reply_msg)) {
			// TODO handle send reply failed
			return FALSE;
		}
		print_skip_list_node(&the_skip_list);
		break;

	case TRAVERSE_SKIP_LIST:
		write_log(TORUS_NODE_LOG, "receive request traverse skip list.\n");
		do_traverse_skip_list(msg);
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

