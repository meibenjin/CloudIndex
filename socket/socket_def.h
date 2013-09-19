/*
 * socket_def.h
 *
 *  Created on: Sep 12, 2013
 *      Author: meibenjin
 */

#ifndef SOCKET_DEF_H_
#define SOCKET_DEF_H_

#include"../utils.h"

//socket API
#include<netinet/in.h>
#include<arpa/inet.h>

// limits for socket
#define MAX_IP_ADDR_LENGTH 20
#define LISTEN_PORT 10086
#define LISTEN_QUEUE_LENGTH 20
#define SOCKET_BUF_SIZE 1024 
#define MAX_DATA_LENGTH 1000   

typedef struct node_info
{
	// torus node ip address
	char ip[MAX_IP_ADDR_LENGTH];
    struct coordinate id;

}node_info;

typedef struct message
{
	uint8_t op;
	char src[MAX_IP_ADDR_LENGTH];
	char dst[MAX_IP_ADDR_LENGTH];
    char data[MAX_DATA_LENGTH];
}message;


#endif /* SOCKET_DEF_H_ */
