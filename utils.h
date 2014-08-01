/*

 *
 *  Created on: Sep 16, 2013
 *      Author: meibenjin
 */

#ifndef UTILS_H_
#define UTILS_H_

#include<time.h>
#include<inttypes.h>

// lock for rtree
#define HAVE_PTHREAD_H 1

// limits for common
#define MAX_FILE_NAME 255 
#define INT_MAX 0x7fffffff
#define PI 3.1415926
#define PRECISION 1e-8
// use day present second
#define ONE_SEC (1.0 /(24 * 60 * 60))

// limits for socket
#define IP_ADDR_LENGTH 20
#define MANUAL_WORKER_PORT 10086
#define COMPUTE_WORKER_PORT 10087
#define LISTEN_QUEUE_LENGTH 20
#define REQUEST_LIST_LENGTH 1024 
#define SOCKET_BUF_SIZE 1024
#define DATA_SIZE 1400
#define SOCKET_ERROR -1
#define STAMP_SIZE 40 

//limits for index 
#define MAX_DIM_NUM 3

// limits for torus
// a torus node's max capacity(pages)
#define DIRECTIONS 6
#define MAX_NEIGHBORS 12
#define MAX_NODES_NUM 1000 
#define MAX_CLUSTERS_NUM 1000 

// limits for skip list
#define LEADER_NUM 8
#define MAXLEVEL 31
#define SKIPLIST_P 0.5

// LOG file path
//#define WRITE_LOG
#define CTRL_NODE_LOG "../logs/control_node.log"
#define TORUS_NODE_LOG "../logs/torus_node.log"
#define RESULT_LOG "../logs/query_result.log"
#define RTREE_LOG "../logs/rtree.log"
#define HEARTBEAT_LOG "../logs/heartbeat.log"
//#define REFINEMENT_LOG "../logs/refinement.log"
#define ERROR_LOG "../logs/error.log"
#define NOTIFY_LOG "../logs/notify.log"

// Data file path
//#define DATA_DIR "./"
#define DATA_DIR "/root/mbj/data"
#define DATA_REGION "data_region"
#define TORUS_IP_LIST "../etc/torus_ip_list"
#define CLUSTER_PARTITONS "./cluster_partitions"
#define TORUS_LEADERS "./torus_leaders"
#define PROPERTIES_FILE "./properties"

//limits for epoll
#define MAX_EVENTS 10000 
#define COMPUTE_WORKER 2
#define MANUAL_WORKER 1
#define EPOLL_NUM (COMPUTE_WORKER + MANUAL_WORKER)
#define WORKER_PER_GROUP 1
#define WORKER_NUM (EPOLL_NUM * WORKER_PER_GROUP)

#define CONN_MAXFD 1024 
#define CONN_BUF_SIZE (SOCKET_BUF_SIZE * 400) 

//#define INT_DATA
//typedef int data_type;
typedef double data_type;

// operation for oct_tree
#define DATA_INSERT 1
#define DATA_DELETE 0
#define RANGE_NN_QUERY 2
#define RANGE_QUERY 3

//global properties 
extern uint32_t DEFAULT_CAPACITY;
extern int HEARTBEAT_INTERVAL;
extern int MAX_ROUTE_STEP;
extern double SIGMA; 
extern int SAMPLE_TIME_POINTS;
extern int SAMPLE_SPATIAL_POINTS;
extern double SAMPLE_TIME_RATE;
extern int MAX_RESPONSE_TIME;
extern double EXCHANGE_RATE_RANGE_QUERY;
extern double EXCHANGE_RATE_NN_QUERY;
extern int ACTIVE_LEADER_NUM;
extern int FIXED_IDLE_NODE_NUM;


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
    THROUGHPUT_TEST,
    RECEIVE_FILTER_LOG,
    RECEIVE_REFINEMENT_LOG,
    RELOAD_RTREE,
    LOAD_OCT_TREE_POINTS,
    LOAD_OCT_TREE_NODES,
    LOAD_OCT_TREE_TRAJECTORYS,
    TRAJ_QUERY,
    RANGE_QUERY_REFINEMENT,
    NN_QUERY_REFINEMENT,
    NOTIFY_MESSAGE,
    RELOAD_PROPERTIES
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
    int fvalue;
}node_stat;


// leaders info
typedef struct leaders_info {
    int cluster_id;
    torus_partitions partition;
    char ip[LEADER_NUM][IP_ADDR_LENGTH];
}leaders_info;

typedef struct socket_st {
    char ip[IP_ADDR_LENGTH];
    int sockfd;
}socket_st;

// used for store trajectory line segments
struct traj_segments{
    uint32_t t_id;
    // the num of pair start and end point
    uint32_t pair_num;
    struct point *start;
    struct point *end;
};

typedef struct refinement_stat {
    uint32_t traj_num;
    // segments num of all candidate trajs
    uint32_t segs_num;
    double avg_time_span;
    uint32_t data_volume;
}refinement_stat;


// structs for logs
typedef struct filter_log_struct {
    int query_id;
    struct interval qry_region[MAX_DIM_NUM];
    int index_elapsed;
    int obtain_idle_elapsed;
    int send_refinement_elapsed;
    int requested_num;
    int actual_got_num;
    uint32_t size_after_index;
    uint32_t size_after_filter;
}filter_log_struct;

typedef struct refinement_log_struct {
    int query_id;
    struct interval qry_region[MAX_DIM_NUM]; 
    int recv_refinement_elapsed;
    int calc_qp_elapsed;
    uint32_t result_size;   
    double avg_qp;
}refinement_log_struct;

typedef struct global_properties_struct {
    uint32_t default_capacity;
    int heartbeat_interval;
    int max_route_step;
    double sigma; 
    int sample_time_points;
    int sample_spatial_points;
    double sample_time_rate;
    int max_response_time;
    double exchange_rate_range_query;
    double exchange_rate_nn_query;
    int active_leader_num;
    int fixed_idle_node_num;
}global_properties_struct;



#endif /* UTILS_H_ */

