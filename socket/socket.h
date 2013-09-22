/*
 * socket_server.h
 *
 *  Created on: Sep 12, 2013
 *      Author: meibenjin
 */

#ifndef SOCKET_SERVER_H_
#define SOCKET_SERVER_H_


//socket API
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include<netinet/in.h>
#include<arpa/inet.h>

#include"../utils.h"

// initial a socket address
void init_socket_addr(struct sockaddr_in *socket_addr);

/* create a new client socket
 *
 * return: socket file descriptor
 *
 */

int new_client_socket();

/* create a new server socket
 *
 * return: socket file descriptor
 *
 */
int new_server_socket();

// accept a connection from client
int accept_connection(int socketfd);

// reply to client
int reply(int socketfd, int reply_code);

// receive message
int send_message(int socketfd, struct message msg);

// receive message
int receive_message(int socketfd, struct message *msg);

// get local ip address based on if_name(eth0, eth1...)
int get_local_ip(char *ip);

void print_message(struct message msg);

int gen_request_stamp(char *stamp);

#endif /* SOCKET_SERVER_H_ */
