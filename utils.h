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


// limits for torus
#define MAX_NEIGHBORS 6
#define MAX_NODES_NUM 20 * 20 * 20

// limits for skip list
#define MAXLEVEL 31

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
	TRAVERSE_TORUS,
    TRAVERSE_SKIP_LIST,
	READ,
	WRITE,
} OP;

// torus node info 
typedef struct node_info {
	// torus node ip address
	char ip[IP_ADDR_LENGTH];

	// cluster id the node belongs to
	int cluster_id;

	// torus node coordinate
	struct coordinate node_id;

} node_info;

typedef struct torus_node {
	// torus node info
	struct node_info info;

	// number of neighbors
	int neighbors_num;

	// torus node's neighbors
	struct torus_node *neighbors[MAX_NEIGHBORS];
} torus_node;


//skip list node structure
typedef struct skip_list_node {
	node_info leader;
    int height;
    struct skip_list_level{
        struct skip_list_node *forward;
        struct skip_list_node *backward;
    }level[];
}skip_list_node;

// header of skip_list
typedef struct skip_list{
	skip_list_node *header;
	int level;
}skip_list;


#endif /* UTILS_H_ */

