/*
 * server.h
 *
 *  Created on: Sep 24, 2013
 *      Author: meibenjin
 */

#ifndef SERVER_H_
#define SERVER_H_

#include"utils.h"

request *new_request();

request *insert_request(request *list, const char *req_stamp);

request *find_request(request *list, const char *req_stamp);

int remove_request(request *list, const char *req_stamp);

// generate a unique request id for each client request
int gen_request_stamp(char *stamp);

// handle the traverse torus request from client
int do_traverse_torus(struct message msg);

int forward_to_neighbors(struct message msg);

int search_rtree(int op, int id, struct interval intval[]);

int do_search_torus_node(struct message msg);

// handle update torus request from client
int do_update_torus(struct message msg);

// handle update skip list node request functions
int do_update_skip_list(struct message msg);

// update current torus node's skip list node's forward and backward
int do_update_skip_list_node(struct message msg);

// update current torus node's skip list node's forward
int do_update_forward(struct message msg);

// update current torus node's skip list node's backward
int do_update_backward(struct message msg);

// search skip list node
int do_search_skip_list_node(struct message msg);

// create a new skip list struct 
int do_new_skip_list(struct message msg); 

// get elasped time from start to end
long get_elasped_time(struct timespec start, struct timespec end);

// get search result ( only for collect result)
int do_receive_result(struct message msg);

// get search query( only for collect result)
int do_receive_query(struct message msg);

// receive data( thread handle
void *do_receive_data(void *args);

/* resolve the message sent from client
 * send reply code to client after all.
 *
 */
int process_message(int socketfd, struct message msg);

// create a new server instance
int new_server();

#endif /* SERVER_H_ */
