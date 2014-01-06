/*
 * utils.h
 *
 *  Created on: Sep 16, 2013
 *      Author: meibenjin
 */

#ifndef UTILS_H_
#define UTILS_H_

// limits for common
#define MAX_FILE_NAME 255 


// limits for socket
#define IP_ADDR_LENGTH 20
#define LISTEN_PORT 10086
#define LISTEN_QUEUE_LENGTH 1024 
#define REQUEST_LIST_LENGTH 20
#define SOCKET_BUF_SIZE 1024 
#define DATA_SIZE 1000
#define SOCKET_ERROR -1
#define STAMP_SIZE 40 

//limits for rtree
#define MAX_DIM_NUM 3

// limits for torus
#define MAX_NEIGHBORS 6
#define MAX_NODES_NUM 20 * 20 * 20

// limits for skip list
#define MAXLEVEL 31
#define SKIPLIST_P 0.5

// LOG file path
#define WRITE_LOG
#define CTRL_NODE_LOG "../logs/control_node.log"
#define TORUS_NODE_LOG "../logs/torus_node.log"
#define RESULT_LOG "../logs/query_result.log"

//limits for epoll
#define MAX_EVENTS 10000

//#define INT_DATA
//typedef int data_type;
typedef double data_type;

// 3-dimension coordinate
typedef struct coordinate {
	int x;
	int y;
	int z;
} coordinate;

// return value of functions
enum status {
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
	UPDATE_PARTITION,
	TRAVERSE_TORUS,
    SEARCH_TORUS_NODE, 
	NEW_SKIP_LIST,
	UPDATE_SKIP_LIST,       // only for control node
	UPDATE_SKIP_LIST_NODE,
	SEARCH_SKIP_LIST_NODE,
	UPDATE_FORWARD,
	UPDATE_BACKWARD,
	TRAVERSE_SKIP_LIST,
	RECEIVE_RESULT,
	RECEIVE_QUERY,
} OP;

// interval of each dimension
typedef struct interval {
	data_type low;
	data_type high;
} interval;

// torus node info 
typedef struct node_info {
	// torus node ip address
	char ip[IP_ADDR_LENGTH];

	// cluster id the node belongs to
	int cluster_id;

	struct interval dims[MAX_DIM_NUM];

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

	// the level of skip list node
	int height;

	/* torus cluster leader node info
	 * note: dims in node_info is the
	 * total interval of whole torus
	 * cluster in all dimension
	 */
	node_info leader;
    struct skip_list_level {
        struct skip_list_node *forward;
        struct skip_list_node *backward;
    }level[];
	//struct skip_list_level level[];

} skip_list_node;

// header of skip_list
typedef struct skip_list {
	skip_list_node *header;
	int level;
} skip_list;


/*
 * definitions for control node
 */

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

// torus partitions on 3-dimension
typedef struct torus_partitions {
	int p_x;
	int p_y;
	int p_z;
} torus_partitions;

typedef struct torus_s {
    torus_partitions partition;
    int cluster_id;
    torus_node *node_list;
}torus_s;

typedef struct torus_cluster {
    struct torus_s *torus;
    struct torus_cluster *next;
} torus_cluster;

/*
 * definitions for torus server 
 */

// request info for torus node
typedef struct request {
	int first_run;
	int receive_num;
	char stamp[STAMP_SIZE];
	struct request *next;
} request;

#endif /* UTILS_H_ */

