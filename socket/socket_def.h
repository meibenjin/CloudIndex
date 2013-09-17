/*
 * socket_def.h
 *
 *  Created on: Sep 12, 2013
 *      Author: meibenjin
 */

#ifndef SOCKET_DEF_H_
#define SOCKET_DEF_H_

#include"../head.h"
//socket API
#include<netinet/in.h>
#include<arpa/inet.h>
//#include<sys/types.h>

#define LISTEN_PORT 10086
#define LISTEN_QUEUE_LENGTH 20
#define SOCKET_BUF_SIZE 8192

typedef struct message{
	uint8_t op;
	char src[MAX_IP_ADDR_LENGTH];
	char dst[MAX_IP_ADDR_LENGTH];
}message;

#endif /* SOCKET_DEF_H_ */
