/*
 * server.h
 *
 *  Created on: Sep 24, 2013
 *      Author: meibenjin
 */

#ifndef SERVER_H_
#define SERVER_H_

#include"utils.h"


// torus server request list
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
struct request *new_request();

struct request *insert_request(struct request *list, const char *req_stamp);

struct request *find_request(struct request *list, const char *req_stamp);

int remove_request(struct request *list, const char *req_stamp);

// generate a unique request id for each client request
int gen_request_stamp(char *stamp);

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

// handle the traverse torus request from client
int do_traverse_torus(struct message msg);

// forward message to current torus node's neighbors
int forward_to_neighbors(struct message msg);

// send part of current torus node's rtree 
int send_splitted_rtree(char *dst_ip, double plow[], double phigh[]);

// recreate local rtree by specified region
int rtree_recreate(double plow[], double phigh[]);

// append a new torus node if current torus node is up to bound
int torus_split();

// query type can be insert, delete or query
int operate_rtree(struct query_struct query);

// query torus nodes
int do_query_torus_node(struct message msg);

// query torus cluster 
int do_query_torus_cluster(struct message msg);

// handle create torus request from client
int do_create_torus(struct message msg);

// handle update skip list node request functions
int do_update_skip_list(struct message msg);

// update current torus node's skip list node's forward and backward
int do_update_skip_list_node(struct message msg);

// create a new skip list struct 
int do_new_skip_list(struct message msg); 

// get elasped time from start to end
long get_elasped_time(struct timespec start, struct timespec end);

// get search result ( only for collect result)
int do_receive_result(struct message msg);

// get search query( only for collect result)
int do_receive_query(struct message msg);

// receive data( thread handle)
void *do_receive_data(void *args);

/* dispatch request based on 
 * the operation code in message
 */
int process_message(connection_t conn, struct message msg);

// create a new server instance
int new_server(int port);

// close a connection
void close_connection(connection_t conn);

// handle epoll read event
int handle_read_event(connection_t conn);

// manual worker monitor thread handler
void *manual_worker_monitor(void *args);

// compute worker monitor thread handler
void *compute_worker_monitor(void *args);

// worker thread threads handler
void *worker(void *args);

// 
void *do_performance_test_long(void *args);

double calc_refinement(struct interval region[], point start, point end);

int find_idle_torus_node(char idle_ip[]);

#endif /* SERVER_H_ */

