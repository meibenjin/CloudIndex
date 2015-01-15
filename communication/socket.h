/*
 * socket.h
 *
 *  Created on: Sep 12, 2013
 *      Author: meibenjin
 */

#ifndef SOCKET_H_
#define SOCKET_H_

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

// send data, with socket fd
int send_data(int socketfd, void *data, size_t len);

int recv_data(int socketfd, void *data, size_t len);

#endif /* SOCKET_SERVER_H_ */


