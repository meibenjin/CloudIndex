/*
 * socket_server.h
 *
 *  Created on: Sep 12, 2013
 *      Author: meibenjin
 */

#ifndef SOCKET_SERVER_H_
#define SOCKET_SERVER_H_

//socket API
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/ioctl.h>
#include<net/if.h>
#include<netinet/in.h>
#include<arpa/inet.h>

#include"utils.h"

/* message for transport among torus nodes
 * op:  operation code
 * src_ip: source ip address
 * dst_ip: destination ip address
 * stamp: unique request stamp for global system
 * data: transport data stream from src_ip to dst_ip
 */
typedef struct message
{
	int op;
	char src_ip[IP_ADDR_LENGTH];
	char dst_ip[IP_ADDR_LENGTH];
	char stamp[STAMP_SIZE];
    char data[DATA_SIZE];
}message;

/* reply message for transport among torus nodes
 * op: operation code
 * stamp unique request stamp for global system
 * reply_code: message process result
 */
typedef struct reply_message{
	OP op;
	char stamp[STAMP_SIZE];
	REPLY_CODE reply_code;
}reply_message;

// initial a socket address
void init_socket_addr(struct sockaddr_in *socket_addr);

/* create a new client socket
 * return: socket file descriptor
 */
int new_client_socket();

/* create a new server socket
 * return: socket file descriptor
 */
int new_server_socket();

// accept a connection from client
int accept_connection(int socketfd);

int send_reply(int socketfd, struct reply_message reply_msg);

int receive_reply(int socketfd, struct reply_message *reply_msg);

int send_message(int socketfd, struct message msg);

void fill_message(OP op, char *src_ip, char *dst_ip, char *stamp, char *data, message *msg);

int forward_message(struct message msg);

int receive_message(int socketfd, struct message *msg);

// get local ip address based on if_name(eth0, eth1...)
int get_local_ip(char *ip);

void print_message(struct message msg);

#endif /* SOCKET_SERVER_H_ */


