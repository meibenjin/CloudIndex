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
#define REQUEST_LIST_LENGTH 20
#define SOCKET_BUF_SIZE 1024 
#define DATA_SIZE 1000
#define REPLY_SIZE sizeof(int)
#define SOCKET_ERROR -1
#define STAMP_SIZE 40 

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
	int op;
	char stamp[STAMP_SIZE];
	int reply_code;
}reply_message;


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

// directions code in 3-dimension
enum direction
{
    X_R = 0,
    X_L,
    Y_R,
    Y_L,
    Z_R,
    Z_L,
};
// return value of functions
enum status
{
	TRUE = 0, 
    FALSE = -1
};

// reply code for the client request
enum reply
{
    SUCCESS = 1,
    FAILED,
    VISITED,
    // incorrect operations 
    WRONG_OP
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



