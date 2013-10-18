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

// 3-dimension coordinate
typedef struct coordinate {
	int x;
	int y;
	int z;
} coordinate;

// return value of functions
enum status{
	TRUE = 0, FALSE = -1
};

// reply code for the client request
typedef enum REPLY_CODE {
	SUCCESS = 1, FAILED, VISITED,
	// incorrect operations
	WRONG_OP
} REPLY_CODE;

//operations for torus nodes in communication 
typedef enum OP {
	// update torus node info
	UPDATE_TORUS = 80,
	UPDATE_SKIP_LIST,
	TRAVERSE,
	READ,
	WRITE,
} OP;

#endif /* UTILS_H_ */

