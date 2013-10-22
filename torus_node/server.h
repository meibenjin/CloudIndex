/*
 * server.h
 *
 *  Created on: Sep 24, 2013
 *      Author: meibenjin
 */

#ifndef SERVER_H_
#define SERVER_H_

#include"utils.h"
#include"torus_node/torus_node.h"
#include"socket/socket.h"

// request info for torus node
typedef struct request {
	int first_run;
	int receive_num;
	char stamp[STAMP_SIZE];
	struct request *next;
} request;

struct request *req_list;

request *new_request();

request *insert_request(request *list, const char *req_stamp);

request *find_request(request *list, const char *req_stamp);

int remove_request(request *list, const char *req_stamp);

// generate a unique request id for each client request
int gen_request_stamp(char *stamp);

// handle the traverse torus request from client
int do_traverse_torus(struct message msg);

int forward_to_neighbors(struct message msg);

// handle the update torus request from client
int do_update_torus(struct message msg);

int do_update_skip_list(struct message msg);

int do_update_skip_list_node(struct message msg);

int do_update_forward(struct message msg);

int do_update_backward(struct message msg);

int do_new_skip_list(struct message msg); 

/* resolve the message sent from client
 * send reply code to client after all.
 *
 */
int process_message(int socketfd, struct message msg);

// create a new server instance
int new_server();

#endif /* SERVER_H_ */
