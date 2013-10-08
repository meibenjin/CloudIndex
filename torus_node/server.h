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

int init_request_list();

int find_request(const char *req_stamp);

int insert_request(const char *req_stamp);

int get_request(const char *req_stamp, struct request *req_ptr);

int remove_request(const char *req_stamp);

// generate a unique request id for each client request
int gen_request_stamp(char *stamp);

// handle the traverse torus request from client
int do_traverse_torus(int socketfd, struct message msg);

int forward_to_neighbors(struct message msg);

// handle the update torus request from client
int do_update_torus(struct message msg);

/* resolve the message sent from client
 * send reply code to client after all.
 *
 */
int process_message(int socketfd, struct message msg);

// create a new server instance
int new_server();


#endif /* SERVER_H_ */
