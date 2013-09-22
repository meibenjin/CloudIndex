/*
 * socket_server.h
 *
 *  Created on: Sep 12, 2013
 *      Author: meibenjin
 */

#ifndef SOCKET_SERVER_H_
#define SOCKET_SERVER_H_


//socket API
#include<netinet/in.h>
#include<arpa/inet.h>

#include"../utils.h"
#include"../torus_node/torus_node.h"

// init server address
int init_server_addr(struct sockaddr_in *server_addr);

// create a new server socket
int new_server_socket();

// accept a connection from client
int accept_connection(int socketfd);

// handle the update torus request from client
int do_update_torus(struct message msg);

/* resolve the message sended by client
 *
 * return: the reply code( SUCCESS, FAILED, WRONG_OP)
 *
 */
int process_message(struct message msg);

// process a request from client 
int process_request(int socketfd);

// start a server socket
int start_server_socket();

void print_message(message msg);

#endif /* SOCKET_SERVER_H_ */
