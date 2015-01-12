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
#include<fcntl.h>
#include<unistd.h>

#include"utils.h"

/* message for transport among torus nodes
 * msg_size: the size of whole message
 * op:  operation code
 * src_ip: source ip address
 * dst_ip: destination ip address
 * stamp: unique request stamp for global system
 * data: transport data stream from src_ip to dst_ip
 */

typedef struct message
{
    size_t msg_size; 
	OP op;
	char src_ip[IP_ADDR_LENGTH];
	char dst_ip[IP_ADDR_LENGTH];
	char stamp[STAMP_SIZE];
    char data[DATA_SIZE];
}message;

typedef struct packet {
    char *data;
    uint32_t data_len;
}packet;

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


/* create a new client socket
 * return: socket file descriptor
 */
int new_client_socket(const char *ip, int port);

/* create a new server socket
 * return: socket file descriptor
 */
int new_server_socket(int port);

// set socket fd no blocking
int set_nonblocking(int socketfd);

int set_blocking(int socketfd);

// accept a connection from client
int accept_connection(int socketfd);


// get local ip address based on if_name(eth0, eth1...)
int get_local_ip(char *ip);

void fill_message(size_t msg_size,  OP op, const char *src_ip, \
                  const char *dst_ip, const char *stamp, const char *data, \
                  size_t data_len, message *msg);

size_t calc_msg_header_size();

void print_message(struct message msg);

// send data, with socket fd
int send_data(int socketfd, void *data, size_t len);

int recv_data(int socketfd, void *data, size_t len);

// send data, without socket fd
int send_message(struct message msg);

int receive_message(int socketfd, struct message *msg);

int send_reply(int socketfd, struct reply_message reply_msg);

int receive_reply(int socketfd, struct reply_message *reply_msg);

#endif /* SOCKET_SERVER_H_ */


