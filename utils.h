/*
 * utils.h
 *
 *  Created on: Sep 16, 2013
 *      Author: meibenjin
 */

#ifndef UTILS_H_
#define UTILS_H_


// limits for socket
#define IP_ADDR_LENGTH 20
#define LISTEN_PORT 10086
#define LISTEN_QUEUE_LENGTH 20
#define SOCKET_BUF_SIZE 1024 
#define DATA_SIZE 1000
#define REPLY_SIZE sizeof(int)
#define SOCKET_ERROR -1

// message for communication among torus nodes
typedef struct message
{
	int op;
	char src_ip[IP_ADDR_LENGTH];
	char dst_ip[IP_ADDR_LENGTH];
    char data[DATA_SIZE];
}message;

// limits for torus
#define MAX_NEIGHBORS 6
#define MAX_NODES_NUM 10 * 10 * 10


// 3-dimension coordinate
typedef struct coordinate
{
	int x;
	int y;
	int z;
}coordinate;

// return value of functions
enum status
{
	TRUE = 0, FALSE = -1
};

// reply code for the client request
enum reply
{
    SUCCESS = 1,
    FAILED,
    // incorrect operations 
    WRONG_OP
};

// torus node status
enum torus_node_status
{
	BUSY = 1,
	FREE = 0
};
    
//operations for torus nodes in communication 
enum OP
{
    // update torus node info
    UPDATE_TORUS = 80,
    TRAVERSE,
    READ, 
    WRITE,
};

#endif /* UTILS_H_ */



