/*
 * utils.h
 *
 *  Created on: Sep 16, 2013
 *      Author: meibenjin
 */

#ifndef UTILS_H_
#define UTILS_H_

#include<time.h>

// lock for rtree
#define HAVE_PTHREAD_H 1

// limits for common
#define MAX_FILE_NAME 255 

// limits for socket
#define IP_ADDR_LENGTH 20
#define MANUAL_WORKER_PORT 10086
#define COMPUTE_WORKER_PORT 10087
#define LISTEN_QUEUE_LENGTH 20
#define REQUEST_LIST_LENGTH 1024 
#define SOCKET_BUF_SIZE 1024
#define DATA_SIZE 1000
#define SOCKET_ERROR -1
#define STAMP_SIZE 40 
#define PI 3.1415926

//limits for rtree
#define MAX_DIM_NUM 3

// limits for torus
// a torus node's max capacity(pages)
#define DEFAULT_CAPACITY 20000000
//#define DEFAULT_CAPACITY 3000
#define DIRECTIONS 6
#define MAX_NEIGHBORS 10
#define MAX_NODES_NUM 1000 
#define MAX_CLUSTERS_NUM 1000 
#define HEARTBEAT_INTERVAL 5
#define WORKLOAD_THRESHOLD 2
#define MAX_ROUTE_STEP 3
#define REFINEMENT_THRESHOLD 0.8

// standard deviation sigma
//#define SIGMA 0.00005
#define SIGMA 900 
#define PRECISION 1e-8

// use day present second
#define ONE_SEC (1.0 /(24 * 60 * 60))

// limits for skip list
#define LEADER_NUM 3
#define MAXLEVEL 31
#define SKIPLIST_P 0.5

// LOG file path
//#define WRITE_LOG
#define CTRL_NODE_LOG "../logs/control_node.log"
#define TORUS_NODE_LOG "../logs/torus_node.log"
#define RESULT_LOG "../logs/query_result.log"
#define RTREE_LOG "../logs/rtree.log"
#define HEARTBEAT_LOG "../logs/heartbeat.log"
#define REFINEMENT_LOG "../logs/refinement.log"

// Data file path
//#define DATA_DIR "./"
#define DATA_DIR "/root/mbj/data"
#define DATA_REGION "data_region"
#define TORUS_IP_LIST "../etc/torus_ip_list"
#define CLUSTER_PARTITONS "./cluster_partitions"
#define TORUS_LEADERS "./torus_leaders"

//limits for epoll
#define MAX_EVENTS 10000 
#define COMPUTE_WORKER 4 
#define MANUAL_WORKER 1
#define EPOLL_NUM (COMPUTE_WORKER + MANUAL_WORKER)
#define WORKER_PER_GROUP 1
#define WORKER_NUM (EPOLL_NUM * WORKER_PER_GROUP)
#define CONN_MAXFD 10240 
#define CONN_BUF_SIZE (SOCKET_BUF_SIZE * 32) 

//#define INT_DATA
//typedef int data_type;
typedef double data_type;

// operation for rtree
#define RTREE_INSERT 1
#define RTREE_DELETE 0
#define RTREE_QUERY 2

// 3-dimension coordinate
typedef struct coordinate {
	int x;
	int y;
	int z;
} coordinate;


typedef struct point {
    data_type axis[MAX_DIM_NUM];
}point;

typedef struct traj_point {
    struct point p;
    int t_id;
}traj_point;


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
	CREATE_TORUS = 80,
	UPDATE_PARTITION,
	TRAVERSE_TORUS,
    QUERY_TORUS_NODE, 
	QUERY_TORUS_CLUSTER,
    LOAD_DATA,
	NEW_SKIP_LIST,
	UPDATE_SKIP_LIST,       // only for control node
	UPDATE_SKIP_LIST_NODE,
	TRAVERSE_SKIP_LIST,
    SEEK_IDLE_NODE,
    HEARTBEAT,
	RECEIVE_RESULT,
	RECEIVE_QUERY,
    RECEIVE_DATA, 
    PERFORMANCE_TEST,
    RELOAD_RTREE,
    LOAD_OCT_TREE_POINTS,
    LOAD_OCT_TREE_NODES,
    LOAD_OCT_TREE_TRAJECTORYS,
    TRAJ_QUERY,
    RANGE_QUERY_REFINEMENT,
    NN_QUERY_REFINEMENT
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

	struct interval region[MAX_DIM_NUM];

	// torus node coordinate
	struct coordinate node_id;

    // torus node's capacity(number of datas)
    unsigned int capacity;

} node_info;

// torus neighbors node information
struct neighbor_node{
    struct node_info *info;
    struct neighbor_node *next;
};

typedef struct torus_node {
	// torus node info
	struct node_info info;

    // leader flag
    int is_leader;

	// number of neighbors
	int neighbors_num;

	// torus node's neighbors in 6 directions
    struct neighbor_node *neighbors[DIRECTIONS]; 

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
	node_info leader[LEADER_NUM];
    struct skip_list_level {
        struct skip_list_node *forward;
        struct skip_list_node *backward;
    }level[];

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
    X_L = 0,
    X_R,
    Y_L,
    Y_R,
    Z_L,
    Z_R,
};

// torus partitions on 3-dimension
typedef struct torus_partitions {
	int p_x;
	int p_y;
	int p_z;
} torus_partitions;

// a torus cluster info
// include: cluster_id, partition, leaders and node_list
typedef struct torus_s {
    int cluster_id;
    torus_partitions partition;
    node_info leaders[LEADER_NUM];
    torus_node *node_list;
}torus_s;

// linked list of torus clusters
typedef struct torus_cluster {
    struct torus_s *torus;
    struct torus_cluster *next;
} torus_cluster;

// struct of a query, include insert, delete and query
typedef struct query_struct {
    int op;
    int data_id;
    int trajectory_id;
    interval intval[MAX_DIM_NUM];
} query_struct;

// struct connections
typedef struct connection_st {
    int socketfd;
    // which epoll fd this conn belongs to
    int index; 
    // flag
    int used;
    // time when this socket to be used; 
    struct timespec enter_time;
    //read offset
    int roff;
    char rbuf[CONN_BUF_SIZE];
}*connection_t;

// status of current torus node
// only max_wait_time now
typedef struct node_stat {
    char ip[IP_ADDR_LENGTH];
    long max_wait_time;
}node_stat;

// used for refinement
typedef struct line_segment {
    int t_id;
    // the num of pair start and end point
    int pair_num;
    struct point *start;
    struct point *end;
}line_segment ;

// leaders info
typedef struct leaders_info {
    int cluster_id;
    torus_partitions partition;
    char ip[LEADER_NUM][IP_ADDR_LENGTH];
}leaders_info;


#endif /* UTILS_H_ */

