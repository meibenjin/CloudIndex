/*
 * socket_server.h
 *
 *  Created on: Sep 12, 2013
 *      Author: meibenjin
 */

#ifndef SOCKET_SERVER_H_
#define SOCKET_SERVER_H_

#include"socket_def.h"

// init server address
int init_server_addr(struct sockaddr_in *server_addr);

// create a new server socket
int new_server_socket();

// accept a connection from client
int accept_connection(int socketfd);

// process a request from client 
int process_request(int socketfd);

// start a server socket
int start_server_socket();

void print_message(message msg);

#endif /* SOCKET_SERVER_H_ */
