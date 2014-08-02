/*
 * server.c
 *
 *  Created on: Sep 24, 2013
 *      Author: meibenjin
 */

__asm__(".symver memcpy,memcpy@GLIBC_2.2.5");

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/time.h>
#include<time.h>
#include<unistd.h>
#include<math.h>
#include<sys/epoll.h>
#include<pthread.h>
#include<errno.h>

extern "C" {
#include"logs/log.h"
#include"torus_node/torus_node.h"
#include"skip_list/skip_list.h"
#include"socket/socket.h"
#include"config/config.h"
}; 

#include"server.h"
#include"torus_rtree.h"
#include"oct_tree/OctTree.h"


#include"torus_rtree.h"
#include "gsl_rng.h"
#include "gsl_randist.h"


//define torus server 
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/* note: some of these global variables are used for torus leaders 
 * some are used for torus nodes, and some used for both two */

OctTree *the_torus_oct_tree;

// mark torus node is active or not 
int should_run;

torus_node *the_torus;
struct node_stat the_node_stat = {"", 0};
struct skip_list *the_skip_list;
node_info the_torus_leaders[LEADER_NUM];
struct torus_partitions the_partition;
ISpatialIndex* the_torus_rtree;

// if current torus node is a leader node(this means it is a skip list node)
// create socket for this node to it's forward and backward neighbor at every level 
int sln_forward_sockfd[MAXLEVEL][LEADER_NUM];
int sln_backward_sockfd[MAXLEVEL][LEADER_NUM];

// array of long socket from current node to it's leaders
struct socket_st leader_sock[LEADER_NUM];

// array of long socket from current node to it's neighbor
struct socket_st neighbor_sock[MAX_NEIGHBORS];
int neighbor_sock_num;

// connections queue 
struct connection_st g_conn_table[CONN_MAXFD];

// worker epoll fds
// these epoll fds handle ready connections from client
int worker_epfd[EPOLL_NUM];

// socket for accept requests from client
int manual_worker_socket;

// socket for accept compute or data transform task from client
int compute_worker_socket;

// multiple threads in torus node
pthread_t manual_worker_monitor_thread;
pthread_t compute_worker_monitor_thread;
pthread_t worker_thread[WORKER_NUM];
pthread_t heartbeat_thread;

// mutex variables
pthread_mutex_t mutex;
pthread_mutex_t global_variable_mutex;
//pthread_cond_t condition;

char result_ip[IP_ADDR_LENGTH] = "172.16.0.83";
int result_sockfd;
char result_log[100];
char filter_log[100];
char refinement_log[100];

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

// internal functions 
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
int recreate_trajs(vector<OctPoint *> pt_vector, hash_map<IDTYPE, Traj*> &trajs);
double points_distance(struct point p1, struct point p2);
int calc_QPT(struct point qpt, vector<traj_point *> nn_points, size_t m, hash_map<IDTYPE, double> &rst_map);
int find_neighbor_sock(const char ip[]);
int find_leader_sock(const char ip[]);
int package_refinement_data(hash_map<IDTYPE, Traj*> &trajs, struct query_struct query, char **buf_ptr, uint32_t *data_size, struct refinement_stat *r_stat);
int compute_range_query_stat(hash_map<IDTYPE, Traj*> &trajs, struct query_struct query, struct refinement_stat *r_stat);
int partition_query(struct query_struct query, struct query_struct sub_queries[], int num);
int parse_refinement_head(struct message msg, struct refinement_stat *r_stat, struct query_struct *query, uint32_t *data_size);
int receive_refinement_data(connection_t conn, uint32_t data_size, struct traj_segments traj_segs[], uint32_t traj_num);
int get_center_points(struct point qpt, struct traj_segments traj_segs[], uint32_t traj_num, vector<traj_point *> &center_points);
int send_throw_out_query_msg(struct query_struct query, struct refinement_stat overall_stat);

// given ip , return the index of the_torus_leader 
int leader_nodes_index(const char ip[]);

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

// torus cluster status list 
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

// status of this torus cluster
typedef struct torus_stat {
    struct node_stat node;
    struct torus_stat *next;
}torus_stat;

struct torus_stat *the_torus_stat;

struct torus_stat *new_torus_stat() {
    struct torus_stat *ptr;
    ptr = (struct torus_stat *)malloc(sizeof(struct torus_stat));
    if(ptr == NULL) {
        printf("malloc torus stat list failed.\n");
        return NULL;
    }
    ptr->next = NULL;
    return ptr;
}

struct torus_stat *find_node_stat(struct torus_stat *list, const struct node_stat node_stat) {
	struct torus_stat *ptr = list->next;
	while (ptr) {
		if (strcmp(ptr->node.ip, node_stat.ip) == 0) {
			return ptr;
		}
		ptr = ptr->next;
	}
	return NULL;
}

struct torus_stat *insert_node_stat(struct torus_stat *list, const struct node_stat node_stat) {
	struct torus_stat *new_ts = new_torus_stat();
	if (new_ts == NULL) {
		printf("insert_node_stat: allocate new node_stat failed.\n");
		return NULL;
	}
    strncpy(new_ts->node.ip, node_stat.ip, IP_ADDR_LENGTH);
    new_ts->node.fvalue = node_stat.fvalue;

	struct torus_stat *ptr = list;
	new_ts->next = ptr->next;
	ptr->next = new_ts;

	return new_ts;
}

int remove_node_stat(struct torus_stat *list, const struct node_stat node_stat) {
	struct torus_stat *pre_ptr = list;
	struct torus_stat *ptr = list->next;
	while (ptr) {
		if (strcmp(ptr->node.ip, node_stat.ip) == 0) {
			pre_ptr->next = ptr->next;
			free(ptr);
			ptr = NULL;
			return TRUE;
		}
		pre_ptr = pre_ptr->next;
		ptr = ptr->next;
	}
	return FALSE;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/


// torus server request list
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

// request info for torus node
struct request {
	int first_run;
	int receive_num;
	char stamp[STAMP_SIZE];
	struct request *next;
};

struct request *req_list;

struct request *new_request() {
	request *req_ptr;
	req_ptr = (struct request *) malloc(sizeof(struct request));
	if (req_ptr == NULL) {
		printf("malloc request list failed.\n");
		return NULL;
	}
	req_ptr->first_run = TRUE;
	req_ptr->receive_num = 0;
	memset(req_ptr->stamp, 0, STAMP_SIZE);
	req_ptr->next = NULL;
	return req_ptr;
}

struct request *find_request(struct request *list, const char *req_stamp) {
	struct request *req_ptr = list->next;
	while (req_ptr) {
		if (strcmp(req_ptr->stamp, req_stamp) == 0) {
			return req_ptr;
		}
		req_ptr = req_ptr->next;
	}
	return NULL;
}

struct request *insert_request(struct request *list, const char *req_stamp) {
	struct request *new_req = new_request();
	if (new_req == NULL) {
		printf("insert_request: allocate new request failed.\n");
		return NULL;
	}
	strncpy(new_req->stamp, req_stamp, STAMP_SIZE);

	request *req_ptr = list;
	new_req->next = req_ptr->next;
	req_ptr->next = new_req;

	return new_req;
}

int remove_request(struct request *list, const char *req_stamp) {
	struct request *pre_ptr = list;
	struct request *req_ptr = list->next;
	while (req_ptr) {
		if (strcmp(req_ptr->stamp, req_stamp) == 0) {
			pre_ptr->next = req_ptr->next;
			free(req_ptr);
			req_ptr = NULL;
			return TRUE;
		}
		pre_ptr = pre_ptr->next;
		req_ptr = req_ptr->next;
	}
	return FALSE;
}

int gen_request_stamp(char *stamp) {
	if (stamp == NULL) {
		printf("gen_request_stamp: stamp is null pointer.\n");
		return FALSE;
	}
	static long number_stamp = 1;
	char ip_stamp[IP_ADDR_LENGTH];
    get_node_ip(the_torus->info, ip_stamp);
	/*if (FALSE == get_local_ip(ip_stamp)) {
		return FALSE;
	}*/
	snprintf(stamp, STAMP_SIZE, "%s_%ld", ip_stamp, number_stamp++);
	return TRUE;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/


int find_neighbor_sock(const char ip[]) {
    int i;
    for(i = 0; i < neighbor_sock_num; i++) {
        if(strcmp(neighbor_sock[i].ip, ip) == 0) {
            return neighbor_sock[i].sockfd;
        }
    }
    return FALSE;
}

int find_leader_sock(const char ip[]) {
    int i;
    for(i = 0; i < LEADER_NUM; i++) {
        if(strcmp(leader_sock[i].ip, ip) == 0) {
            return leader_sock[i].sockfd;
        }
    }
    return FALSE;
}

int do_traverse_torus(struct message msg) {
	char stamp[STAMP_SIZE];
	memset(stamp, 0, STAMP_SIZE);

	int neighbors_num = get_neighbors_num(the_torus);

	if (strcmp(msg.stamp, "") == 0) {
		if (FALSE == gen_request_stamp(stamp)) {
			return FALSE;
		}
		strncpy(msg.stamp, stamp, STAMP_SIZE);
		neighbors_num += 1;
	} else {
		strncpy(stamp, msg.stamp, STAMP_SIZE);
	}

	request *req_ptr = find_request(req_list, stamp);

	if (NULL == req_ptr) {
		req_ptr = insert_request(req_list, stamp);
		printf("traverse torus: %s -> %s\n", msg.src_ip, msg.dst_ip);

        #ifdef WRITE_LOG
            //write log
            char buf[1024];
            memset(buf, 0, 1024);
            sprintf(buf, "traverse torus: %s -> %s\n", msg.src_ip, msg.dst_ip);
            write_log(TORUS_NODE_LOG, buf);
        #endif

		if (req_ptr && (req_ptr->first_run == TRUE)) {
			forward_to_neighbors(msg);
			req_ptr->first_run = FALSE;
			req_ptr->receive_num = neighbors_num;
		}

	} else {
		req_ptr->receive_num--;
		if (req_ptr->receive_num == 0) {
			remove_request(req_list, stamp);
		}
	}

	return TRUE;
}

int forward_to_neighbors(struct message msg) {
	int d;
	char src_ip[IP_ADDR_LENGTH], dst_ip[IP_ADDR_LENGTH];

	//int neighbors_num = get_neighbors_num(the_torus);
	get_node_ip(the_torus->info, src_ip);
	for (d = 0; d < DIRECTIONS; ++d) {
        if(the_torus->neighbors[d] != NULL) {
            struct neighbor_node *nn_ptr;
            nn_ptr = the_torus->neighbors[d]->next;
            while(nn_ptr != NULL) {
                get_node_ip(*nn_ptr->info, dst_ip);
                strncpy(msg.src_ip, src_ip, IP_ADDR_LENGTH);
                strncpy(msg.dst_ip, dst_ip, IP_ADDR_LENGTH);
                forward_message(msg, 0);
                nn_ptr = nn_ptr->next;
            }
        }

	}
	return TRUE;
}

int do_create_torus(struct message msg) {
	int i, j, d;
    size_t cpy_len = 0;
	int num, leaders_num, neighbors_num = 0;
    char neighbor_ip[IP_ADDR_LENGTH], leader_ip[IP_ADDR_LENGTH];
    int sockfd;

    // allocate for the_torus if it's NULL
    if(the_torus == NULL) {
        the_torus = new_torus_node();
    }

    // get torus partitons info from msg
    memcpy(&the_partition, (void *)msg.data, sizeof(struct torus_partitions));
    cpy_len += sizeof(struct torus_partitions);

    char buf[1024];
    memset(buf, 0, 1024);
    sprintf(buf, "torus partitions:[%d %d %d]\n", the_partition.p_x,
            the_partition.p_y, the_partition.p_z);
    write_log(TORUS_NODE_LOG, buf);

    // get torus leaders info 
    memcpy(&leaders_num, (void *)(msg.data + cpy_len), sizeof(int));
    cpy_len += sizeof(int);

    for(j = 0; j < leaders_num; j++) {
        memcpy(&the_torus_leaders[j], (void *)(msg.data + cpy_len), sizeof(node_info));
        cpy_len += sizeof(node_info);

        //create long socket fd from current node to it's leaders
        get_node_ip(the_torus_leaders[j], leader_ip);
        sockfd = new_client_socket(leader_ip, MANUAL_WORKER_PORT);
        if(FALSE == sockfd) {
            return FALSE;
        }
        leader_sock[j].sockfd = sockfd;
        strncpy(leader_sock[j].ip, leader_ip, IP_ADDR_LENGTH);

    }
    // write leader node into log file
    print_torus_leaders(the_torus_leaders);


    // get leader flag from msg
    memcpy(&the_torus->is_leader, (void *)(msg.data + cpy_len), sizeof(int));
    cpy_len += sizeof(int);

    // get local torus node from msg
    memcpy(&the_torus->info, (void *)(msg.data + cpy_len), sizeof(node_info));
    cpy_len += sizeof(node_info);

    // set oct tree's region
    // important!
    if(the_torus_oct_tree == NULL) {
        double plow[MAX_DIM_NUM], phigh[MAX_DIM_NUM];

        for (i = 0; i < MAX_DIM_NUM; i++) {
            plow[i] = (double)the_torus->info.region[i].low;
            phigh[i] = (double)the_torus->info.region[i].high;
        }
        the_torus_oct_tree = new OctTree(plow, phigh);
    }

    /* init the_torus_stat and node_stat
     * note: if current torus node is not a leader node
     * the_torus_stat is NULL */
    if(the_torus->is_leader == 0) {
        the_torus_stat = NULL;
    } else {
        the_torus_stat = new_torus_stat();
    }
    strncpy(the_node_stat.ip, the_torus->info.ip, IP_ADDR_LENGTH);
    the_node_stat.fvalue = 0;

    // get neighbors info from msg 
    for(d = 0; d < DIRECTIONS; d++) {
        num = 0;
        memcpy(&num, (void *)(msg.data + cpy_len), sizeof(int));
        cpy_len += sizeof(int); 

        for(i = 0; i < num; i++) {
            // allocate space for the_torus's neighbor at direction d
            torus_node *new_node = new_torus_node();
            if (new_node == NULL) {
                return FALSE;
            }
            memcpy(&new_node->info, (void *)(msg.data + cpy_len), sizeof(node_info));
            cpy_len += sizeof(node_info);

            add_neighbor_info(the_torus, d, &new_node->info);

            //create long socket fd from current node to every neighbor
            get_node_ip(new_node->info, neighbor_ip);
            sockfd = new_client_socket(neighbor_ip, MANUAL_WORKER_PORT);
            if(FALSE == sockfd) {
                return FALSE;
            }
            strncpy(neighbor_sock[neighbor_sock_num].ip, neighbor_ip, IP_ADDR_LENGTH);
            neighbor_sock[neighbor_sock_num].sockfd = sockfd;
            neighbor_sock_num++;

        }
        neighbors_num += num;
    }


    //create long socket fd from current node to result collecter
    result_sockfd = new_client_socket(result_ip, MANUAL_WORKER_PORT);
    if(FALSE == sockfd) {
        return FALSE;
    }

    // set the_torus's neighbors_num
	set_neighbors_num(the_torus, neighbors_num);

    #ifdef WRITE_LOG 
        memset(buf, 0, 1024);
        sprintf(buf, "torus node capacity:%d %d\n", the_torus->info.capacity, neighbor_sock_num);
        write_log(TORUS_NODE_LOG, buf);
    #endif

	return TRUE;
}

int do_update_partition(struct message msg) {

	memcpy(&the_partition, msg.data, sizeof(struct torus_partitions));

    #ifdef WRITE_LOG 
        char buf[1024];
        sprintf(buf, "torus partitions:[%d %d %d]\n", the_partition.p_x,
                the_partition.p_y, the_partition.p_z);
        write_log(TORUS_NODE_LOG, buf);
    #endif

	return TRUE;
}

int do_update_skip_list(struct message msg) {
	int i, j, index, nodes_num, level;
    int cpy_len = 0;

	nodes_num = 0;
	memcpy(&nodes_num, msg.data, sizeof(int));
    cpy_len += sizeof(int);
	if (nodes_num <= 0) {
		printf("do_update_skip_list: skip_list node number is wrong\n");
		return FALSE;
	}

	skip_list_node *sln_ptr, *new_sln;
	node_info nodes[nodes_num][LEADER_NUM];

	for (i = 0; i < nodes_num; ++i) {
        for( j = 0; j < LEADER_NUM; j++) { 
            memcpy(&nodes[i][j], msg.data + cpy_len, sizeof(node_info));
            cpy_len += sizeof(node_info);
        }
	}

	level = (nodes_num / 2) - 1;
	index = 0;
	sln_ptr = the_skip_list->header;

	for (i = level; i >= 0; i--) {
		if (get_cluster_id(nodes[index][0]) != -1) {
			new_sln = new_skip_list_node(0, nodes[index]);
			// free old forward field first
			if (sln_ptr->level[i].forward) {
				free(sln_ptr->level[i].forward);
			}
			sln_ptr->level[i].forward = new_sln;
			index++;
		} else {
			index++;
		}
		if (get_cluster_id(nodes[index][0]) != -1) {
			new_sln = new_skip_list_node(0, nodes[index]);
			// free old forward field first
			if (sln_ptr->level[i].backward) {
				free(sln_ptr->level[i].backward);
			}
			sln_ptr->level[i].backward = new_sln;
			index++;
		} else {
			index++;
		}
	}
	return TRUE;
}

int do_traverse_skip_list(struct message msg) {
	//char buf[1024];
    int i;
	char src_ip[IP_ADDR_LENGTH], dst_ip[IP_ADDR_LENGTH];
	skip_list_node *cur_sln, *forward, *backward;
	cur_sln = the_skip_list->header;
	forward = cur_sln->level[0].forward;
	backward = cur_sln->level[0].backward;

    #ifdef WRITE_LOG
        write_log(TORUS_NODE_LOG, "visit myself:");
    #endif
    //rand choose a leader
    i = rand() % ACTIVE_LEADER_NUM;
    print_node_info(cur_sln->leader[i]);
    get_node_ip(cur_sln->leader[i], src_ip);
    strncpy(msg.src_ip, src_ip, IP_ADDR_LENGTH);
    if (strcmp(msg.data, "") == 0) {
        if (forward) {
            get_node_ip(forward->leader[i], dst_ip);
            strncpy(msg.dst_ip, dst_ip, IP_ADDR_LENGTH);
            strncpy(msg.data, "f", DATA_SIZE);
            forward_message(msg, 0);
        }

        if (backward) {
            get_node_ip(backward->leader[i], dst_ip);
            strncpy(msg.dst_ip, dst_ip, IP_ADDR_LENGTH);
            strncpy(msg.data, "b", DATA_SIZE);
            forward_message(msg, 0);
        }

    } else if (strcmp(msg.data, "f") == 0) {
        if (forward) {
            get_node_ip(forward->leader[i], dst_ip);
            strncpy(msg.dst_ip, dst_ip, IP_ADDR_LENGTH);
            forward_message(msg, 0);
        }

    } else {
        if (backward) {
            get_node_ip(backward->leader[i], dst_ip);
            strncpy(msg.dst_ip, dst_ip, IP_ADDR_LENGTH);
            forward_message(msg, 0);
        }
    }


	return TRUE;
}

int do_update_skip_list_node(struct message msg) {
	int i, j, socketfd, update_forward, update_backword;
    char dst_ip[IP_ADDR_LENGTH];

	node_info f_node[LEADER_NUM], b_node[LEADER_NUM];

    size_t cpy_len = 0;

	// analysis node info from message
    
    // the level of this skip list node
	memcpy(&i, msg.data, sizeof(int));
    cpy_len += sizeof(int);

	memcpy(&update_forward, msg.data + cpy_len, sizeof(int));
    cpy_len += sizeof(int);
	memcpy(&update_backword, msg.data + cpy_len, sizeof(int));
    cpy_len += sizeof(int);

    if(update_forward == 1) {
        memcpy(f_node, (void*)(msg.data + cpy_len), sizeof(node_info) * LEADER_NUM);
        cpy_len += sizeof(node_info) * LEADER_NUM;
    }

    if(update_backword == 1) {
        memcpy(b_node, (void*)(msg.data + cpy_len), sizeof(node_info) * LEADER_NUM);
        cpy_len += sizeof(node_info) * LEADER_NUM;
    }

	skip_list_node *sln_ptr, *new_sln;
	sln_ptr = the_skip_list->header;

    if(update_forward == 1) {
		new_sln = new_skip_list_node(0, f_node);

		// free old forward field first
		if (sln_ptr->level[i].forward) {
			free(sln_ptr->level[i].forward);
		}

		sln_ptr->level[i].forward = new_sln;

        // create long socket fd for current node to its forward neighbors
        for(j = 0; j < LEADER_NUM; j++) {
            get_node_ip(sln_ptr->level[i].forward->leader[j], dst_ip);
            socketfd = new_client_socket(dst_ip, MANUAL_WORKER_PORT);
            if (FALSE == socketfd) {
                return FALSE;
            }
            sln_forward_sockfd[i][j] = socketfd;
        }
	}

    if(update_backword == 1) {
		new_sln = new_skip_list_node(0, b_node);

		// free old backward field first
		if (sln_ptr->level[i].backward) {
			free(sln_ptr->level[i].backward);
		}
		sln_ptr->level[i].backward = new_sln;

        // create long socket fd for current node to its forward neighbors
        for(j = 0; j < LEADER_NUM; j++) {
            get_node_ip(sln_ptr->level[i].backward->leader[j], dst_ip);
            socketfd = new_client_socket(dst_ip, MANUAL_WORKER_PORT);
            if (FALSE == socketfd) {
                return FALSE;
            }
            sln_backward_sockfd[i][j] = socketfd;
        }
	}
	return TRUE;
}

int do_new_skip_list(struct message msg) {
	int level, i;
    size_t cpy_len = 0;

	// get node info from message
	skip_list_node *sln_ptr;
	node_info leaders[LEADER_NUM];

	memcpy(&level, msg.data, sizeof(int));
    cpy_len += sizeof(int);
	memcpy(leaders, msg.data + cpy_len, sizeof(node_info) * LEADER_NUM);
    cpy_len += sizeof(node_info) * LEADER_NUM;

    if(the_skip_list == NULL) {
        the_skip_list = new_skip_list(level);
    }

    sln_ptr = the_skip_list->header;
    for(i = 0; i < LEADER_NUM; ++i) {
        sln_ptr->leader[i] = leaders[i];
    }

    sln_ptr->height = level;
    the_skip_list->level = level;
	return TRUE;
}

int forward_search(int op, struct interval intval[], struct message msg, int d) {
	char src_ip[IP_ADDR_LENGTH], dst_ip[IP_ADDR_LENGTH];

    // the curent torus node ip
    get_node_ip(the_torus->info, src_ip);

	struct coordinate lower_id = get_node_id(the_torus->info);
	struct coordinate upper_id = get_node_id(the_torus->info);

    //TODO get the neighbors of current torus node
	switch (d) {
	case 0:
		lower_id.x = (lower_id.x + the_partition.p_x - 1) % the_partition.p_x;
		upper_id.x = (upper_id.x + the_partition.p_x + 1) % the_partition.p_x;
		break;
	case 1:
		lower_id.y = (lower_id.y + the_partition.p_y - 1) % the_partition.p_y;
		upper_id.y = (upper_id.y + the_partition.p_y + 1) % the_partition.p_y;
		break;
	case 2:
		lower_id.z = (lower_id.z + the_partition.p_z - 1) % the_partition.p_z;
		upper_id.z = (upper_id.z + the_partition.p_z + 1) % the_partition.p_z;
		break;
	}

	node_info *lower_neighbor = get_neighbor_by_id(the_torus, lower_id);
	node_info *upper_neighbor = get_neighbor_by_id(the_torus, upper_id);

    int lower_overlap = interval_overlap(intval[d], lower_neighbor->region[d]);
    int upper_overlap = interval_overlap(intval[d], upper_neighbor->region[d]);

    // choose forward neighbor if query overlap one neighbor at most
    if ((lower_overlap != 0) || (upper_overlap != 0)) {
        if(lower_neighbor != NULL && upper_neighbor != NULL) {
            //current torus node is the first node on dimension d
            if (is_first_torus_node(the_torus->info, *lower_neighbor, d)) {
                if (get_distance(intval[d], lower_neighbor->region[d])
                        < get_distance(intval[d], upper_neighbor->region[d])) {
                    upper_neighbor = lower_neighbor;
                }
                lower_neighbor = NULL;

            //current torus node is the last node on dimension d
            } else if (is_last_torus_node(the_torus->info, *upper_neighbor, d)) {
                if (get_distance(intval[d], upper_neighbor->region[d])
                        < get_distance(intval[d], lower_neighbor->region[d])) {
                    lower_neighbor = upper_neighbor;
                }
                upper_neighbor = NULL;

            } else {
                //the query region is on the lower side of current torus node's region
                if(-1 == interval_overlap(intval[d], the_torus->info.region[d])) {
                    upper_neighbor = NULL;
                //the query region is on the upper side of current torus node's region
                } else if(1 == interval_overlap(intval[d], the_torus->info.region[d])) {
                    lower_neighbor = NULL;
                }
            }
        }
    }

	// the message is from lower_neighbor
	if ((lower_neighbor != NULL) && strcmp(lower_neighbor->ip, msg.src_ip) == 0) {
		lower_neighbor = NULL;
	}
	// the message is from upper_neighbor
	if ((upper_neighbor != NULL) && strcmp(upper_neighbor->ip, msg.src_ip) == 0) {
		upper_neighbor = NULL;
	}

	// the lower_neighbor is itself
	if ((lower_neighbor != NULL) && strcmp(lower_neighbor->ip, src_ip) == 0) {
		lower_neighbor = NULL;
	}
	// the upper_neighbor is itself
	if ((upper_neighbor != NULL) && strcmp(upper_neighbor->ip, src_ip) == 0) {
		upper_neighbor = NULL;
	}

    if(lower_neighbor ==  upper_neighbor) {
        lower_neighbor = NULL;
    }

    char buf[1024];
    int socketfd;
    memset(buf, 0, 1024);
	if (lower_neighbor != NULL) {
		struct message new_msg;
		get_node_ip(*lower_neighbor, dst_ip);
		fill_message((OP)msg.op, src_ip, dst_ip, msg.stamp, msg.data, &new_msg);

        #ifdef WRITE_LOG
            sprintf(buf, "forward to %s\n", dst_ip);
            write_log(TORUS_NODE_LOG, buf);
        #endif
		//forward_message(new_msg, 0);
        socketfd = find_neighbor_sock(dst_ip);
        if(FALSE != socketfd) {
            send_safe(socketfd, (void *) &new_msg, sizeof(struct message), 0);
        }
	}
	if (upper_neighbor != NULL) {
		struct message new_msg;
		get_node_ip(*upper_neighbor, dst_ip);
		fill_message((OP)msg.op, src_ip, dst_ip, msg.stamp, msg.data, &new_msg);

        #ifdef WRITE_LOG
            sprintf(buf, "forward to %s\n", dst_ip);
            write_log(TORUS_NODE_LOG, buf);
        #endif
		//forward_message(new_msg, 0);
        socketfd = find_neighbor_sock(dst_ip);
        if(FALSE != socketfd) {
            send_safe(socketfd, (void *) &new_msg, sizeof(struct message), 0);
        }
        
	}
	return TRUE;
}

torus_node *append_torus_node(int d){
    //TODO get new node info from control node
    struct node_info *info_ptr;

    torus_node *new_node = new_torus_node();
    if(new_node == NULL) {
        return NULL;
    }
    info_ptr = &new_node->info;

	set_node_ip(info_ptr, "172.16.0.167");
	set_node_id(info_ptr, 2, 1, 1);
	set_cluster_id(info_ptr, the_torus->info.cluster_id);
    set_node_capacity(info_ptr, DEFAULT_CAPACITY);

    return new_node;
}

int get_split_dimension() {
    int d = 0, i, j;
    double plow[MAX_DIM_NUM], phigh[MAX_DIM_NUM];
    size_t balance_gain = -1, min_gain = -1; 
    size_t pnum = 0, nnum = 0;

    size_t c = rtree_get_utilization(the_torus_rtree);
    char buf[1024];
    memset(buf, 0, 1024);
    int len = sprintf(buf, "%lu ", c);
    for(i = 0; i < MAX_DIM_NUM; i++) {
        for (j = 0; j < MAX_DIM_NUM; j++) {
            plow[j] = (double)the_torus->info.region[j].low;
            phigh[j] = (double)the_torus->info.region[j].high;
        }
        plow[i] = (plow[i] + phigh[i]) * 0.5;
        pnum = rtree_query_count(plow, phigh, the_torus_rtree);
        nnum = c - pnum; 
        balance_gain = (pnum > nnum) ? (pnum - nnum) : (nnum - pnum);
        if(balance_gain < min_gain) {
            min_gain = balance_gain;
            d = i;
        }
        len += sprintf(buf + len, "%f ", plow[i]);
        len += sprintf(buf + len, "%lu %lu ", pnum, nnum);
    }
    len += sprintf(buf + len, "%d\n", d);
    write_log(RTREE_LOG, buf);
    return d;
}

int send_splitted_rtree(char *dst_ip, double plow[], double phigh[]) {

    char buffer[1024];
    struct timespec start, end, s, e;
    double elasped = 0L, el;
    clock_gettime(CLOCK_REALTIME, &start);
    MyVisitor vis;
    rtree_range_query(plow, phigh, the_torus_rtree, vis);

    std::vector<SpatialIndex::IData*> v = vis.GetResults();
    std::vector<SpatialIndex::IData*>::iterator it;

    clock_gettime(CLOCK_REALTIME, &end);
    elasped = get_elasped_time(start, end);
    memset(buffer, 0, 1024);
    sprintf(buffer, "get split data spend %f ms\n", (double) elasped/ 1000000.0);
    write_log(RESULT_LOG, buffer);
    elasped = 0L;

	int socketfd;

    // create a data socket to send rtree data
	socketfd = new_client_socket(dst_ip, COMPUTE_WORKER_PORT);
	if (FALSE == socketfd) {
		return FALSE;
	}

	// get local ip address
	char local_ip[IP_ADDR_LENGTH];
	memset(local_ip, 0, IP_ADDR_LENGTH);
    get_node_ip(the_torus->info, local_ip);
	/*if (FALSE == get_local_ip(local_ip)) {
		return FALSE;
	}*/

    struct message msg;
    msg.op = RELOAD_RTREE;
    strncpy(msg.src_ip, local_ip, IP_ADDR_LENGTH);
    strncpy(msg.dst_ip, dst_ip, IP_ADDR_LENGTH);
    strncpy(msg.stamp, "", STAMP_SIZE);
    strncpy(msg.data, "", DATA_SIZE);
    send_safe(socketfd, (void *)&msg, sizeof(struct message), 0);
    // important: wait server close epoll mode
    /*struct reply_message reply_msg;
    if( TRUE == receive_reply(socketfd, &reply_msg)) {
        if (SUCCESS != reply_msg.reply_code) {
            return FALSE;
        }
    }*/

    size_t cpy_len = sizeof(uint32_t);
    int total_len = 0; 
    uint32_t count = 0;
    RTree::Data *data;

    char buf[SOCKET_BUF_SIZE];
    memset(buf, 0, SOCKET_BUF_SIZE);

    for(it = v.begin(); it != v.end(); it++) {
        clock_gettime(CLOCK_REALTIME, &s);

        // data include data_id region and data content
        data = dynamic_cast<RTree::Data *>(*it);
        byte *package_ptr;
        uint32_t package_len;
        data->storeToByteArray(&package_ptr, package_len);

        // data num
        count++;
        memcpy(buf + 0, &count, sizeof(uint32_t));

        // package_len may be unused in do_rtree_load_data() need more test
        //memcpy(buf + cpy_len, &package_len, sizeof(uint32_t));
        //cpy_len += sizeof(uint32_t);

        memcpy(buf + cpy_len, package_ptr, package_len);
        cpy_len += package_len;

        delete[] package_ptr;

        clock_gettime(CLOCK_REALTIME, &e);
        el += get_elasped_time(s, e);

        uint32_t next_len = sizeof(uint32_t) * 2 + package_len;
        if(cpy_len + next_len < SOCKET_BUF_SIZE) {
            continue;
        } else {
            clock_gettime(CLOCK_REALTIME, &start);
            send_safe(socketfd, (void *)buf, SOCKET_BUF_SIZE, 0);
            clock_gettime(CLOCK_REALTIME, &end);
            elasped += get_elasped_time(start, end);

            total_len += cpy_len;
            count = 0;
            cpy_len = sizeof(uint32_t);
            memset(buf, 0, SOCKET_BUF_SIZE);
        }
    }
    if(cpy_len > sizeof(uint32_t)) {
        send_safe(socketfd, (void *)buf, SOCKET_BUF_SIZE, 0);
    }
	close(socketfd);

    total_len += cpy_len;
    memset(buffer, 0, 1024);
    sprintf(buffer, "total send %f k\n", (double) total_len/ 1000.0);
    write_log(RESULT_LOG, buffer);

    memset(buffer, 0, 1024);
    sprintf(buffer, "package split data spend %f ms\n", (double) el/ 1000000.0);
    write_log(RESULT_LOG, buffer);

    memset(buffer, 0, 1024);
    sprintf(buffer, "send split data spend %f ms\n", (double) elasped/ 1000000.0);
    write_log(RESULT_LOG, buffer);
    return TRUE;
}


int rtree_recreate(double plow[], double phigh[]){

    MyVisitor vis;
    rtree_range_query(plow, phigh, the_torus_rtree, vis);
    std::vector<SpatialIndex::IData*> v = vis.GetResults();

    // delete origional rtree 
    rtree_delete(the_torus_rtree);

    the_torus_rtree = rtree_bulkload(vis.GetResults());
    return TRUE;
}

int send_oct_points(const char *dst_ip, hash_map<int, OctPoint *> &points) {

    char buffer[1024];
    struct timespec start, end, s, e;
    double elasped = 0L, el;

	int socketfd;

    // create a data socket to send rtree data
	socketfd = new_client_socket(dst_ip, COMPUTE_WORKER_PORT);
	if (FALSE == socketfd) {
		return FALSE;
	}

	// get local ip address
	char local_ip[IP_ADDR_LENGTH];
	memset(local_ip, 0, IP_ADDR_LENGTH);
    get_node_ip(the_torus->info, local_ip);
	/*if (FALSE == get_local_ip(local_ip)) {
		return FALSE;
	}*/

    struct message msg;
    msg.op = LOAD_OCT_TREE_POINTS;
    strncpy(msg.src_ip, local_ip, IP_ADDR_LENGTH);
    strncpy(msg.dst_ip, dst_ip, IP_ADDR_LENGTH);
    strncpy(msg.stamp, "", STAMP_SIZE);
    strncpy(msg.data, "", DATA_SIZE);
    send_safe(socketfd, (void *)&msg, sizeof(struct message), 0);

    // begin to send oct tree points to dst_ip
    
    char buf[SOCKET_BUF_SIZE];
    memset(buf, 0, SOCKET_BUF_SIZE);

    // the first sizeof(uint32_t) bytes save the number of points in buf
    size_t cpy_len = sizeof(uint32_t);
    int total_len = 0; 
    uint32_t points_num = 0;

    hash_map<int, OctPoint *>::iterator it;
    for(it = points.begin(); it != points.end(); it++) {
        clock_gettime(CLOCK_REALTIME, &s);

        char *package_ptr;
        uint32_t package_len;

        it->second->storeToByteArray(&package_ptr, package_len);

        /* one or more points will be filled into buf
         * points_num used to mark the num of points in buf now
         * note: points_num use 0 ~ sizeof(uint32_t) of buf
         */
        points_num++;
        memcpy(buf + 0, &points_num, sizeof(uint32_t));

        //memcpy(buf + cpy_len, &package_len, sizeof(uint32_t));
        //cpy_len += sizeof(uint32_t);

        memcpy(buf + cpy_len, package_ptr, package_len);
        cpy_len += package_len;

        delete[] package_ptr;

        clock_gettime(CLOCK_REALTIME, &e);
        el += get_elasped_time(s, e);

        if(cpy_len + package_len < SOCKET_BUF_SIZE) {
            continue;
        } else {
            clock_gettime(CLOCK_REALTIME, &start);
            send_safe(socketfd, (void *)buf, SOCKET_BUF_SIZE, 0);
            clock_gettime(CLOCK_REALTIME, &end);
            elasped += get_elasped_time(start, end);

            total_len += cpy_len;
            points_num = 0;
            cpy_len = sizeof(uint32_t);
            memset(buf, 0, SOCKET_BUF_SIZE);
        }
    }
    if(cpy_len > sizeof(uint32_t)) {
        send_safe(socketfd, (void *)buf, SOCKET_BUF_SIZE, 0);
    }
	close(socketfd);

    total_len += cpy_len;
    memset(buffer, 0, 1024);
    sprintf(buffer, "total send %f k\n", (double) total_len/ 1000.0);
    write_log(RESULT_LOG, buffer);

    memset(buffer, 0, 1024);
    sprintf(buffer, "package split oct tree points spend %f ms\n", (double) el/ 1000000.0);
    write_log(RESULT_LOG, buffer);

    memset(buffer, 0, 1024);
    sprintf(buffer, "send split oct tree points spend %f ms\n", (double) elasped/ 1000000.0);
    write_log(RESULT_LOG, buffer);

    return TRUE;
}

int send_oct_nodes(const char *dst_ip, hash_map<int, OctTNode *> &nodes) {

    char buffer[1024];
    struct timespec start, end, s, e;
    double elasped = 0L, el;

	int socketfd;

    // create a data socket to send rtree data
	socketfd = new_client_socket(dst_ip, COMPUTE_WORKER_PORT);
	if (FALSE == socketfd) {
		return FALSE;
	}

	// get local ip address
	char local_ip[IP_ADDR_LENGTH];
	memset(local_ip, 0, IP_ADDR_LENGTH);
	if (FALSE == get_local_ip(local_ip)) {
		return FALSE;
	}

    struct message msg;
    msg.op = LOAD_OCT_TREE_NODES;
    strncpy(msg.src_ip, local_ip, IP_ADDR_LENGTH);
    strncpy(msg.dst_ip, dst_ip, IP_ADDR_LENGTH);
    strncpy(msg.stamp, "", STAMP_SIZE);
    strncpy(msg.data, "", DATA_SIZE);
    send_safe(socketfd, (void *)&msg, sizeof(struct message), 0);

    // begin to send oct tree points to dst_ip
    
    char buf[SOCKET_BUF_SIZE];
    memset(buf, 0, SOCKET_BUF_SIZE);

    // the first sizeof(uint32_t) bytes save the number of nodes in buf
    size_t cpy_len = sizeof(uint32_t);
    int total_len = 0; 
    uint32_t nodes_num = 0;


    hash_map<int, OctTNode *>::iterator it;
    for(it = nodes.begin(); it != nodes.end(); it++) {
        clock_gettime(CLOCK_REALTIME, &s);

        char *package_ptr;
        uint32_t package_len;

        it->second->storeToByteArray(&package_ptr, package_len);

        /* one or more points will be filled into buf
         * nodes_num used to mark the num of points in buf now
         * note: nodes_num use 0 ~ sizeof(uint32_t) of buf
         */
        nodes_num++;
        memcpy(buf + 0, &nodes_num, sizeof(uint32_t));

        //memcpy(buf + cpy_len, &package_len, sizeof(uint32_t));
        //cpy_len += sizeof(uint32_t);

        memcpy(buf + cpy_len, package_ptr, package_len);
        cpy_len += package_len;

        delete[] package_ptr;

        clock_gettime(CLOCK_REALTIME, &e);
        el += get_elasped_time(s, e);

        if(cpy_len + package_len < SOCKET_BUF_SIZE) {
            continue;
        } else {
            clock_gettime(CLOCK_REALTIME, &start);
            send_safe(socketfd, (void *)buf, SOCKET_BUF_SIZE, 0);
            clock_gettime(CLOCK_REALTIME, &end);
            elasped += get_elasped_time(start, end);

            total_len += cpy_len;
            nodes_num = 0;
            cpy_len = sizeof(uint32_t);
            memset(buf, 0, SOCKET_BUF_SIZE);
        }
    }
    if(cpy_len > sizeof(uint32_t)) {
        send_safe(socketfd, (void *)buf, SOCKET_BUF_SIZE, 0);
    }
	close(socketfd);

    total_len += cpy_len;
    memset(buffer, 0, 1024);
    sprintf(buffer, "total send %f k\n", (double) total_len/ 1000.0);
    write_log(RESULT_LOG, buffer);

    memset(buffer, 0, 1024);
    sprintf(buffer, "package split oct tree nodes spend %f ms\n", (double) el/ 1000000.0);
    write_log(RESULT_LOG, buffer);

    memset(buffer, 0, 1024);
    sprintf(buffer, "send split oct tree nodes spend %f ms\n", (double) elasped/ 1000000.0);
    write_log(RESULT_LOG, buffer);

    return TRUE;
}

int send_oct_trajectorys(const char *dst_ip, hash_map<IDTYPE, Traj *> &trajs) {

    char buffer[1024];
    struct timespec start, end, s, e;
    double elasped = 0L, el;

	int socketfd;

    // create a data socket to send rtree data
	socketfd = new_client_socket(dst_ip, COMPUTE_WORKER_PORT);
	if (FALSE == socketfd) {
		return FALSE;
	}

	// get local ip address
	char local_ip[IP_ADDR_LENGTH];
	memset(local_ip, 0, IP_ADDR_LENGTH);
	if (FALSE == get_local_ip(local_ip)) {
		return FALSE;
	}

    struct message msg;
    msg.op = LOAD_OCT_TREE_TRAJECTORYS;
    strncpy(msg.src_ip, local_ip, IP_ADDR_LENGTH);
    strncpy(msg.dst_ip, dst_ip, IP_ADDR_LENGTH);
    strncpy(msg.stamp, "", STAMP_SIZE);
    strncpy(msg.data, "", DATA_SIZE);
    send_safe(socketfd, (void *)&msg, sizeof(struct message), 0);

    // begin to send oct tree points to dst_ip
    
    char buf[SOCKET_BUF_SIZE];
    memset(buf, 0, SOCKET_BUF_SIZE);

    // the first sizeof(uint32_t) bytes save the number of trajectorys in buf
    size_t cpy_len = sizeof(uint32_t);
    int total_len = 0; 
    uint32_t trajs_num = 0;


    hash_map<IDTYPE, Traj *>::iterator it;
    for(it = trajs.begin(); it != trajs.end(); it++) {
        clock_gettime(CLOCK_REALTIME, &s);

        char *package_ptr;
        uint32_t package_len;

        it->second->storeToByteArray(&package_ptr, package_len);

        /* one or more trajs will be filled into buf
         * trajs_num used to mark the num of points in buf now
         * note: trajs_num use 0 ~ sizeof(uint32_t) of buf
         */
        trajs_num++;
        memcpy(buf + 0, &trajs_num, sizeof(uint32_t));

        //memcpy(buf + cpy_len, &package_len, sizeof(uint32_t));
        //cpy_len += sizeof(uint32_t);

        memcpy(buf + cpy_len, package_ptr, package_len);
        cpy_len += package_len;

        delete[] package_ptr;

        clock_gettime(CLOCK_REALTIME, &e);
        el += get_elasped_time(s, e);

        if(cpy_len + package_len < SOCKET_BUF_SIZE) {
            continue;
        } else {
            clock_gettime(CLOCK_REALTIME, &start);
            send_safe(socketfd, (void *)buf, SOCKET_BUF_SIZE, 0);
            clock_gettime(CLOCK_REALTIME, &end);
            elasped += get_elasped_time(start, end);

            total_len += cpy_len;
            trajs_num = 0;
            cpy_len = sizeof(uint32_t);
            memset(buf, 0, SOCKET_BUF_SIZE);
        }
    }
    if(cpy_len > sizeof(uint32_t)) {
        send_safe(socketfd, (void *)buf, SOCKET_BUF_SIZE, 0);
    }
	close(socketfd);

    total_len += cpy_len;
    memset(buffer, 0, 1024);
    sprintf(buffer, "total send %f k\n", (double) total_len/ 1000.0);
    write_log(RESULT_LOG, buffer);

    memset(buffer, 0, 1024);
    sprintf(buffer, "package split oct tree trajectorys spend %f ms\n", (double) el/ 1000000.0);
    write_log(RESULT_LOG, buffer);

    memset(buffer, 0, 1024);
    sprintf(buffer, "send split oct tree trajectorys spend %f ms\n", (double) elasped/ 1000000.0);
    write_log(RESULT_LOG, buffer);

    return TRUE;
}

int torus_split() {
    // Step 1: append a new torus node and reset regions
    // get the optimal split dimension
    char buffer[1024];

    struct timespec start, end;
    double elasped = 0L;
    clock_gettime(CLOCK_REALTIME, &start);

    //int d = get_split_dimension();
    double low[MAX_DIM_NUM], high[MAX_DIM_NUM];
    int d; 
    // get best cut dimension, 1 means getLow
    // low and high is the split region
    the_torus_oct_tree->setDom(low, high, 1, d);

    clock_gettime(CLOCK_REALTIME, &end);
    elasped = get_elasped_time(start, end);
    memset(buffer, 0, 1024);
    sprintf(buffer, "get_split_dimension spend %d %f ms\n", d, (double) elasped/ 1000000.0);
    write_log(RESULT_LOG, buffer);

    // append a new torus node
    // TODO get free ip from control node
    torus_node *new_node = append_torus_node(d);

    // reset the regions(both current node and new append torus node
    int i;
    for (i = 0; i < MAX_DIM_NUM; i++) {
        new_node->info.region[i].low = low[i];
        new_node->info.region[i].high = high[i];
    }
    the_torus->info.region[d].low = (the_torus->info.region[d].low + the_torus->info.region[d].high) * 0.5; 

    // Step 2: update neighbor information
    clock_gettime(CLOCK_REALTIME, &start);

    //copy torus partitions into buf(if necessary)
    size_t cpy_len = 0;
    char buf[DATA_SIZE];
    memcpy(buf, (void *)&the_partition, sizeof(struct torus_partitions));
    cpy_len += sizeof(struct torus_partitions);

    //copy leaders info into buf
    int leaders_num = LEADER_NUM;
    memcpy(buf + cpy_len, &leaders_num, sizeof(int));
    cpy_len += sizeof(int);
    for(i = 0; i < leaders_num; i++) {
        memcpy(buf + cpy_len, &the_torus_leaders[i], sizeof(node_info));
        cpy_len += sizeof(node_info);
    }
    
    // copy neighbors info into buf 
    // then send to new torus node
    memcpy(buf + cpy_len,(void *) &new_node->info, sizeof(node_info));
    cpy_len += sizeof(node_info);
    for(i = 0; i < DIRECTIONS; i++) {
        int num = get_neighbors_num_d(the_torus, i);

        // 2*d + 1 represent torus node's upper direction 
        if(i == 2 * d + 1) {
            num++;
        }
        memcpy(buf + cpy_len, &num, sizeof(int));
        cpy_len += sizeof(int); 

        if(the_torus->neighbors[i] != NULL) {
            struct neighbor_node *nn_ptr;
            nn_ptr = the_torus->neighbors[i]->next;
            while(nn_ptr != NULL) {
                memcpy(buf + cpy_len, nn_ptr->info, sizeof(node_info));
                cpy_len += sizeof(node_info);
                nn_ptr = nn_ptr->next;
            }
        }

        // copy the_torus info into buf at dimension d
        // d is the dimension, (include 2 directions d low and d high)
        // 2*d represent torus node's lower direction
        // 2*d + 1 represent torus node's upper direction 
        if(i == 2 * d + 1) {
            // append the_torus node info to new_torus's upper direction of dimension d
            memcpy(buf + cpy_len, &the_torus->info, sizeof(node_info));
            cpy_len += sizeof(node_info);
        }
    }

    char dst_ip[IP_ADDR_LENGTH];
    memset(dst_ip, 0, IP_ADDR_LENGTH);
    get_node_ip(new_node->info, dst_ip);

    // send to dst_ip
    if (FALSE == send_data(CREATE_TORUS, dst_ip, buf, cpy_len)) {
        return FALSE;
    }

    // append new torus node info to the_torus at lower direction of dimension d
    add_neighbor_info(the_torus, 2*d, &new_node->info);

    clock_gettime(CLOCK_REALTIME, &end);
    elasped = get_elasped_time(start, end);
    memset(buffer, 0, 1024);
    sprintf(buffer, "update neighbor information spend %f ms\n", (double) elasped/ 1000000.0);
    write_log(RESULT_LOG, buffer);

    print_torus_node(*the_torus);

    // Step 3: copy oct tree to new torus node and update local oct tree
    clock_gettime(CLOCK_REALTIME, &start);

    // split oct_tree
    the_torus_oct_tree->treeSplit(1);

    // send split oct tree
    send_oct_points(dst_ip, point_list);
    send_oct_nodes(dst_ip, node_list);
    send_oct_trajectorys(dst_ip, traj_list);

    hash_map<int, OctPoint *>::iterator pit;
    hash_map<int, OctTNode *>::iterator nit;
    hash_map<int, Traj *>::iterator tit;
    for(pit = point_list.begin(); pit != point_list.end(); pit++) {
        OctPoint *point = pit->second;
        delete point;
    }
    point_list.clear();
    for(nit = node_list.begin(); nit != node_list.end(); nit++) {
        OctTNode *node = nit->second;
        delete node;
    }
    node_list.clear();

    for(tit = traj_list.begin(); tit != traj_list.end(); tit++) {
        Traj *traj = tit->second;
        delete traj;
    }
    traj_list.clear();

    // recreate local oct tree
    the_torus_oct_tree->treeSplit(0);

    // set oct tree region 
    the_torus_oct_tree->tree_domLow[d] = (the_torus_oct_tree->tree_domLow[d] + the_torus_oct_tree->tree_domHigh[d]) * 0.5;

    // delete origional oct tree
    for(pit = g_PtList.begin(); pit != g_PtList.end(); pit++) {
        OctPoint *point = pit->second;
        delete point;
    }
    g_PtList.clear();

    for(nit = g_NodeList.begin(); nit != g_NodeList.end(); nit++) {
        OctTNode *node = nit->second;
        delete node;
    }
    g_NodeList.clear();

    for(tit = g_TrajList.begin(); tit != g_TrajList.end(); tit++) {
        Traj *traj = tit->second;
        delete traj;
    }
    g_TrajList.clear();

    for(pit = point_list.begin(); pit != point_list.end(); pit++) {
        g_PtList.insert(std::pair<IDTYPE, OctPoint *>(pit->second->p_id, pit->second));
    }
    point_list.clear();

    for(nit = node_list.begin(); nit != node_list.end(); nit++) {
        g_NodeList.insert(std::pair<IDTYPE, OctTNode *>(nit->second->n_id, nit->second));
    }
    node_list.clear();

    for(tit = traj_list.begin(); tit != traj_list.end(); tit++) {
        g_TrajList.insert(std::pair<IDTYPE, Traj *>(tit->second->t_id, tit->second));
    }
    traj_list.clear();

    clock_gettime(CLOCK_REALTIME, &end);
    elasped = get_elasped_time(start, end);
    memset(buffer, 0, 1024);
    sprintf(buffer, "send split oct tree spend %f ms\n", (double) elasped/ 1000000.0);
    write_log(RESULT_LOG, buffer);

    return TRUE;
}

int get_idle_torus_node(char idle_ip[][IP_ADDR_LENGTH], int requested_num, int *actual_got_num) {

    // this means current torus node is not a leader
    if(the_torus_stat == NULL) {
        *actual_got_num = 0;
        return FALSE;
    }

    struct node_stat cur_torus_stat[MAX_NODES_NUM];
    int torus_num = 0;

    struct torus_stat *ptr = the_torus_stat->next;
    while(ptr) {
        cur_torus_stat[torus_num] = ptr->node;
        torus_num++;
        ptr = ptr->next;
    }

    // shuffle the torus stat
    int i, j;
    struct node_stat tmp;
    for(i = torus_num; i > 0; --i) {
        j = rand() % i;
        //swap
        tmp = cur_torus_stat[i - 1];
        cur_torus_stat[i - 1] = cur_torus_stat[j];
        cur_torus_stat[j] = tmp;
    }

    int idle_idx = 0;
    for(i = 0; i < torus_num; i++) {
        if(cur_torus_stat[i].fvalue < MAX_RESPONSE_TIME) {
            strncpy(idle_ip[idle_idx], cur_torus_stat[i].ip, IP_ADDR_LENGTH);
            idle_idx++;
            if(idle_idx == requested_num) {
                break;
            }
        }
    }

    *actual_got_num = idle_idx;
    return TRUE;
}

int local_rtree_query(double low[], double high[]) {
    MyVisitor vis;
    rtree_range_query(low, high, the_torus_rtree, vis);
    std::vector<SpatialIndex::IData*> v = vis.GetResults();

    //find a idle torus node to do refinement
    char idle_ip[IP_ADDR_LENGTH];
    memset(idle_ip, 0, IP_ADDR_LENGTH);
    //find_idle_torus_node(idle_ip);

    return TRUE;
}

int operate_rtree(struct query_struct query) {

    if(the_torus_rtree == NULL) {
        #ifdef WRITE_LOG
            write_log(TORUS_NODE_LOG, "torus rtree didn't create.\n");
        #endif
        return FALSE;
    }

    int i;
    double plow[MAX_DIM_NUM], phigh[MAX_DIM_NUM];

    for (i = 0; i < MAX_DIM_NUM; i++) {
        plow[i] = (double)query.intval[i].low;
        phigh[i] = (double)query.intval[i].high;
    }

    // rtree operation 
    if(query.op == DATA_INSERT) {
        // need lock when insert rtree
        pthread_mutex_lock(&mutex);
        if(FALSE == rtree_insert(query.trajectory_id, query.data_id, plow, phigh, the_torus_rtree)) {
            return FALSE;
        }
        size_t c = rtree_get_utilization(the_torus_rtree);
        if(c >= the_torus->info.capacity){
            torus_split();
        }
        pthread_mutex_unlock(&mutex);
    } else if(query.op == DATA_DELETE) {
        if(FALSE == rtree_delete(query.data_id, plow, phigh, the_torus_rtree)) {
            return FALSE;
        }
    } else if(query.op == RANGE_QUERY) {
        // TODO add read lock
        if(FALSE == local_rtree_query(plow, phigh)) {
            return FALSE;
        }
    }
    return TRUE;
}

// get pre point of pt in trajectory pt->t_id by range query
int traj_range_query(double *low, double *high, IDTYPE t_id, OctPoint *pt, vector<OctPoint*> &pt_vector) {
    int i;
    struct query_struct query;
    for(i = 0; i < MAX_DIM_NUM; i++) {
        query.intval[i].low = low[i];
        query.intval[i].high = high[i];
    }
    query.trajectory_id = t_id;
    query.data_id = -1;
    query.op = RANGE_QUERY;

    int socketfd;
	// get local ip address
	char local_ip[IP_ADDR_LENGTH];
	char dst_ip[IP_ADDR_LENGTH];

	memset(local_ip, 0, IP_ADDR_LENGTH);
	if (FALSE == get_local_ip(local_ip)) {
		return FALSE;
	}

    int cpy_len = 0;
    struct message msg;
    msg.op = TRAJ_QUERY;
    strncpy(msg.stamp, "", STAMP_SIZE);
    memcpy(msg.data, &query, sizeof(struct query_struct));
    cpy_len += sizeof(struct query_struct);

    // send the point to neighbor
    char *package_ptr;
    uint32_t package_len;

    pt->storeToByteArray(&package_ptr, package_len);
    memcpy(msg.data + cpy_len, package_ptr, package_len);
    delete[] package_ptr;

    char buf[SOCKET_BUF_SIZE], *ptr;
    memset(buf, 0, SOCKET_BUF_SIZE);
    int has_point = 0;

    for( i = 0; i < DIRECTIONS; i++) {
        if(the_torus->neighbors[i] != NULL) {
            struct neighbor_node *nn_ptr;
            nn_ptr = the_torus->neighbors[i]->next;
            while(nn_ptr != NULL) {
                node_info *node = nn_ptr->info;
                if (1 == overlaps(query.intval, node->region)) {

                    strncpy(dst_ip, node->ip, IP_ADDR_LENGTH);
                    // create a data socket to send rtree data
                    //socketfd = new_client_socket(dst_ip, MANUAL_WORKER_PORT);
                    socketfd = find_neighbor_sock(dst_ip);
                    if (FALSE == socketfd) {
                        return FALSE;
                    }
                    strncpy(msg.src_ip, local_ip, IP_ADDR_LENGTH);
                    strncpy(msg.dst_ip, dst_ip, IP_ADDR_LENGTH);

                    send_safe(socketfd, (void *)&msg, sizeof(struct message), 0);

                    // get return points vector
                    if(recv_safe(socketfd, buf, SOCKET_BUF_SIZE, 0) == SOCKET_BUF_SIZE) {
                        ptr = buf;
                        memcpy(&has_point, ptr, sizeof(int));
                        ptr += sizeof(int);
                        if(1 == has_point) {
                            OctPoint *point = new OctPoint();
                            point->loadFromByteArray(ptr);
                            pt_vector.push_back(point);
                        }
                    }
                    memset(buf, 0, SOCKET_BUF_SIZE);
                }
                nn_ptr = nn_ptr->next;
            }
        }
    } 
    return TRUE;
}

int oct_tree_insert(OctPoint *pt) {
	OctTNode *root = g_NodeList.find(the_torus_oct_tree->tree_root)->second;
	if (root->containPoint(pt)) {
		//1.以这个点做rangequery
		//2.到于查到的点，不在本节点，且tid与这个点相同，需要做面上的插值
		//3.更新两个server的前后继值
		//4.更新两个server上的trajectory状态
		//这个点所在Trajectory不存在
        int traj_exist = 0;
		if (g_TrajList.find(pt->p_tid) == g_TrajList.end()) {

			vector<OctPoint*> pt_vector;
			//benjin rangequery
			/*double range[3] = { 1.0, 1.0, 1.0 };  //需要定义一下，以这个点为中心的一个范围
			double low[3], high[3];
			for (int i = 0; i < 3; i++) {
				low[i] = pt->p_xyz[i] - range[i];
				high[i] = pt->p_xyz[i] + range[i];
			}*/
            //traj_range_query(low, high, pt->p_tid, pt, pt_vector);//得到包含与其相邻点的vector  ToDo Benjin
			//rangeQuery(low, high, pt_vector);  
			for (size_t i = 0; i < pt_vector.size(); i++) {
				if (pt_vector[i]->p_tid == pt->p_tid &&   //相同Traj
						pt_vector[i]->p_xyz[2] < pt->p_xyz[2] && //在其前
						pt_vector[i]->next == 0                         //next为空
								) {
                    traj_exist = 1;
					OctPoint *point_new = new OctPoint();
					g_ptNewCount++;
					the_torus_oct_tree->geneBorderPoint(pt_vector[i], pt, point_new, the_torus_oct_tree->tree_domLow, the_torus_oct_tree->tree_domHigh);      //求出与面的交点

					point_new->p_id = -g_ptNewCount;
					point_new->p_tid = pt->p_tid;                  //补全信息
					pt->pre = point_new->p_id;
					point_new->next = pt->p_id;
					point_new->pre = 0;                          //更新相关的next和pre

					g_PtList.insert(
							pair<IDTYPE, OctPoint*>(point_new->p_id,
									point_new));

					Traj *t = new Traj(pt->p_tid, pt->p_id, pt->p_id); // 新建一个trajectory
					g_TrajList.insert(pair<int, Traj*>(pt->p_tid, t));

					// TODO (liudehai#1#): 本金将point_new
					//邻居server data域里 并 更新所在Traj的tail
				}
			}
            if(traj_exist == 0) {
                root->nodeInsert(pt);
                g_ptCount++;
                Traj *t = new Traj(pt->p_tid, pt->p_id, pt->p_id); // 新建一个trajectory
                g_TrajList.insert(pair<int, Traj*>(pt->p_tid, t));
            }
		}

		else {

			// 找到所在Trajectory的尾部，并更新相关信息
			Traj *tmp = g_TrajList.find(pt->p_tid)->second;
			/*
			 *near函数用来判断是否是同一个点，这一步用来压缩输入数据用
			 if(pt->isNear(tmp->t_tail)){
			 return;
			 }
			 */
            if(g_PtList.find(tmp->t_tail) != g_PtList.end()){
			    g_PtList.find(tmp->t_tail)->second->next = pt->p_id;
            }
            else{
                printNodes();
            }
            pt->pre = tmp->t_tail;
			tmp->t_tail = pt->p_id;
            root->nodeInsert(pt);
            g_ptCount++;
		}
	} else {
		cout << "point not in this oct-tree!" << endl;
	}
    return TRUE;
}

int leader_nodes_index(const char ip[]) {
    int i;
    for(i = 0; i < LEADER_NUM; i++) {
        if(strcmp(ip, the_torus_leaders[i].ip) == 0) {
            return i;
        }
    }
    return -1;
}

// this function used to judge current torus node
// is a active leader, a active leader is first i
// a leader node, and in current global properties,
// it will play as a true leader.
// the LEADER_NUM - ACTIVE_LEADER_NUM will be the backup leader
int is_active_leader() {
    if(the_torus->is_leader == 0) {
        return FALSE;
    } else {
        int index = leader_nodes_index(the_torus->info.ip);
        if(index != -1  && index < ACTIVE_LEADER_NUM) {
            return TRUE;
        }
    }
    return FALSE;
}

int find_idle_torus_node(char idle_ip[][IP_ADDR_LENGTH], int requested_num, int* actual_got_num) {
    /* check if the_torus itself is a idle torus node
     * then check if the_torus is a leader node, if true, find a 
     * idle node base on the_torus_stat; if not, send SEEK_IDLE_NODE
     * message to its leader
     * */
    *actual_got_num = 0;

    if (is_active_leader() == TRUE){
        //this node is leader
        get_idle_torus_node(idle_ip, requested_num, actual_got_num);
    }else{
        pthread_mutex_lock(&global_variable_mutex);
        int cur_fvalue = the_node_stat.fvalue;
        pthread_mutex_unlock(&global_variable_mutex);

        int idle_idx = 0;
        if(cur_fvalue < MAX_RESPONSE_TIME) {
            strncpy(idle_ip[idle_idx], the_torus->info.ip, IP_ADDR_LENGTH);
            idle_idx++;
            requested_num--;
        }
        if(requested_num == 0) {
            *actual_got_num = idle_idx;
            return TRUE;
        }
        
        // random choose a leader;
        int index = rand() % ACTIVE_LEADER_NUM;
        char *src_ip = the_torus->info.ip;
        char *dst_ip = the_torus_leaders[index].ip;

        int socketfd;
        socketfd = find_leader_sock(dst_ip);
        //socketfd = new_client_socket(dst_ip, MANUAL_WORKER_PORT);
        if (FALSE == socketfd) {
            write_log(ERROR_LOG, "find_idle_torus_node: find leader sock failed.\n");
            return FALSE;
        }

        int route_count = MAX_ROUTE_STEP;
        struct message msg;
        msg.op = SEEK_IDLE_NODE;
        strncpy(msg.src_ip, src_ip, IP_ADDR_LENGTH);
        strncpy(msg.dst_ip, dst_ip, IP_ADDR_LENGTH);
        strncpy(msg.stamp, "", STAMP_SIZE);
        memcpy(msg.data, &route_count, sizeof(int));
        memcpy(msg.data + sizeof(int), &requested_num, sizeof(int));
        send_message(socketfd, msg);

        struct message reply_msg;
        if(TRUE == receive_message(socketfd, &reply_msg)) {
            int cpy_len = 0;
            int ret_num = 0;
            memcpy(&ret_num, reply_msg.data, sizeof(int));
            cpy_len += sizeof(int);
            int i;
            for (i=0;i<ret_num;i++){
                memcpy(idle_ip[idle_idx], reply_msg.data + cpy_len, IP_ADDR_LENGTH);            
                cpy_len += IP_ADDR_LENGTH; 
                idle_idx++;
            }
            *actual_got_num = idle_idx;
        } else {
            *actual_got_num = idle_idx;
            write_log(ERROR_LOG, "find_idle_torus_node: receive seek idle ip reply failed.\n");
            return FALSE;
        }
    }
    return TRUE;
}

/* given a query point with (x, y, z) and a trajs list, 
 * this function output the intersect points list; 
 * for each traj and given query time t, we can find the intersect point of line z = t
 * and line segment in the traj.
 * for example qpt = (5, 5, 9), one of traj = (0,0,0)-(2,2,5)-(4,4,10)-(6,6,15)
 * the intersect point of line z = 9 and line segment(here is (2,2,5)-(4,4,10) ) 
 * of the trajs is one of the output.
 * */
int get_center_points(struct point qpt, struct traj_segments traj_segs[], uint32_t traj_num, vector<traj_point *> &center_points) {
    double z;
    uint32_t i, traj_idx, pair_idx;
    point start, end;
    int has_intersection;

    for(traj_idx = 0; traj_idx < traj_num; traj_idx++) {
        has_intersection = FALSE;
        for(pair_idx = 0; pair_idx < traj_segs[traj_idx].pair_num; pair_idx++) {
            for(i = 0; i < MAX_DIM_NUM; i++) {
                start.axis[i] = traj_segs[traj_idx].start[pair_idx].axis[i];
                end.axis[i] = traj_segs[traj_idx].end[pair_idx].axis[i];
            }

            if(qpt.axis[2] > start.axis[2] && qpt.axis[2] < end.axis[2]) {
                z = qpt.axis[2];
                traj_point *tpt = (traj_point *) malloc(sizeof(traj_point));
                tpt->t_id = traj_segs[traj_idx].t_id;
                tpt->p.axis[0] = ((z - start.axis[2]) * end.axis[0] + (end.axis[2] - z) * start.axis[0]) / \
                                 (end.axis[2] - start.axis[2]);
                tpt->p.axis[1] = ((z - start.axis[2]) * end.axis[1] + (end.axis[2] - z) * start.axis[1]) / \
                                 (end.axis[2] - start.axis[2]);
                tpt->p.axis[2] = z;
                center_points.push_back(tpt);
                has_intersection = TRUE;
            }

            if(has_intersection == TRUE) {
                break;
            }
        }
    }
    return TRUE;
}

int get_nn_oct_points(struct point query_point, vector<traj_point *> &nn_points) {
    // step 1 find nearest neighbor of query_point
    double min_dis, dis;
    size_t i, size;
    size = nn_points.size();
    if(size == 0) {
        return FALSE;
    }

    min_dis = points_distance(query_point, nn_points[0]->p);
    for(i = 1; i < size; i++) {
        dis = points_distance(query_point, nn_points[i]->p);
        if(dis < min_dis) {
            min_dis = dis;
        }
    }

    // delete the points that the distance from query_point
    // is larger then extend_r
    double extend_r = min_dis + 6 * SIGMA;
    vector<traj_point *>::iterator it;
    for(it = nn_points.begin(); it != nn_points.end();) {
        dis = points_distance(query_point, (*it)->p);
        if(dis > extend_r) {
            free(*it);
            *it = NULL;
            it = nn_points.erase(it);
        } else {
            it++;
        }
    }
    return TRUE;
}

double calc_distance(double lata_p, double lona_p, double latb_p, double lonb_p) {
    double lata = lata_p * PI / 180;
    double lona = lona_p * PI / 180;
    double latb = latb_p * PI / 180;
    double lonb = lonb_p * PI / 180;
    double distance = 2 * asin(sqrt(pow(sin((lata-latb)/2), 2) + cos(lata) * cos(latb) * pow(sin((lona - lonb)/2), 2))) * 6378;
    return distance;
}

/* given a query, calc qp of each candidate trajs.
 * */
int calc_nn_query_refinement(struct query_struct query, struct traj_segments traj_segs[], uint32_t traj_num, double *avg_qp) {
    // step 1: generate time samples points 
    // init query point
    struct point query_point;
    query_point.axis[0] = (query.intval[0].low + query.intval[0].high) / 2.0;
    query_point.axis[1] = (query.intval[1].low + query.intval[1].high) / 2.0;

    //calc the size of time samples points
    size_t time_samples = 0;
    double time_step = SAMPLE_TIME_RATE;
    double tmp_size = ceil((query.intval[2].high - query.intval[2].low) / time_step);
    time_samples = (size_t)(tmp_size) + 1; 

    /* for each sampled time between time low and time high 
     * find the nearest neighbor object for query object
     * and calc the qp at time t
     * */ 
    IDTYPE t_id;
    double qp, qpt;
    hash_map<IDTYPE, double> qp_map;
    hash_map<IDTYPE, double>::iterator it;

    size_t i;
    double time = query.intval[2].low; 
    for(i = 1; i <= time_samples; i++) {
        if( i ==  time_samples) {
            time = query.intval[2].high;
        }
        query_point.axis[2] = time;
        time += time_step;

        vector<traj_point *> nn_points;
        vector<traj_point *>::iterator nit;

        // get center points for each trajs at time t
        get_center_points(query_point, traj_segs, traj_num, nn_points);

        // get candidate nearest neighbor objects
        get_nn_oct_points(query_point, nn_points);

        hash_map<IDTYPE, double> qpt_map;
        calc_QPT(query_point, nn_points, SAMPLE_SPATIAL_POINTS, qpt_map);

        for(it = qpt_map.begin(); it != qpt_map.end(); it++) {
            t_id = it->first;
            qpt = it->second;

            //meets a new trajectory
            if(qp_map.find(t_id) == qp_map.end()) {
                qp_map.insert(pair<IDTYPE, double>(t_id, 1.0));
            }
            qp = qp_map.find(t_id)->second;
            if(qpt > 1){
                qp = 0;
            } else {
                qp *= (1 - qpt);
            }
            qp_map.find(t_id)->second = qp;

        }

        // free vector nn_points's memory
        for(nit = nn_points.begin(); nit != nn_points.end();) {
            free(*nit);
            *nit = NULL;
            nit = nn_points.erase(nit);
        }
    }

    double total_qp = 0.0;
    for(it = qp_map.begin(); it != qp_map.end(); it++) {
        t_id = it->first;
        qp = it->second;
        qp = 1 - qp;
        if(qp < 0) {
            qp = 0.0;
        }
        total_qp += qp;
    }

    if(traj_num == 0) {
        *avg_qp = 0.0;
    } else {
        *avg_qp = total_qp / traj_num;
    }
    return TRUE;
}

double points_distance(struct point p1, struct point p2) {
    double distance = 0;
    int i;
    for(i = 0; i < MAX_DIM_NUM; i++) {
        distance += (p1.axis[i] - p2.axis[i]) * (p1.axis[i] - p2.axis[i]); 
    }
    return sqrt(distance);
}

int Comp(const void *p1,const void *p2, void *p0){
    point qpt = *(point *)p0;
    double dis1 = points_distance(*(point *)p1, qpt);
    double dis2 = points_distance(*(point *)p2, qpt);
    if(dis1 < dis2) {
        return -1;
    } else {
        return 1;
    }
}


int calc_QPT(struct point qpt, vector<traj_point *> nn_points, size_t m, hash_map<IDTYPE, double> &rst_map) {
    size_t i, j, traj_cnt;
    double value;
    hash_map<IDTYPE, double> cdf;

    // init gsl lib
    gsl_rng *r;
    gsl_rng_env_setup();
    gsl_rng_default_seed = ((unsigned long)(time(NULL)));
    r = gsl_rng_alloc(gsl_rng_default);

    traj_point point;

    traj_cnt = nn_points.size();
    traj_point * samples;
    samples = (traj_point *) malloc(sizeof(traj_point) * traj_cnt * m);
    if (samples == NULL){
        //err handling
        write_log(ERROR_LOG, "calc_QPT: malloc failed!\n");
        return FALSE;
    }
    uint32_t sample_num = 0;
    for (i = 0; i < traj_cnt; i++) {
        for(j = 0; j < m; j++) {
            point.t_id = nn_points[i]->t_id;
            point.p.axis[2] = nn_points[i]->p.axis[2];

            value = gsl_ran_gaussian(r, SIGMA); 
            while(value < -3 * SIGMA || value > 3 * SIGMA) {
                value = gsl_ran_gaussian(r, SIGMA); 
                continue;
            }
            point.p.axis[0] = nn_points[i]->p.axis[0] + value;

            value = gsl_ran_gaussian(r, SIGMA); 
            while(value < -3 * SIGMA || value > 3 * SIGMA) {
                value = gsl_ran_gaussian(r, SIGMA); 
                continue;
            }
            point.p.axis[1] = nn_points[i]->p.axis[1] + value;
            
            samples[sample_num] = point;
            sample_num++;
        }

        cdf.insert(pair<IDTYPE, double>(nn_points[i]->t_id, 0.0));
        rst_map.insert(pair<IDTYPE, double>(nn_points[i]->t_id, 0.0));
    }
    gsl_rng_free(r);

    //sort the samples asscendingly by distance to qpt
    qsort_r(samples, sample_num, sizeof(samples[0]), Comp, (void *)&qpt);

    // calc qpt
    int t_id = 0;
    double f = 1.0 / m, F;
    for(i = 0; i < sample_num; i++) {
        t_id = samples[i].t_id;

        F = f;
        for (j = 0; j < traj_cnt; j++) {
            if(t_id != nn_points[j]->t_id) {
                F *= 1 - cdf.find(nn_points[j]->t_id)->second;
                if(F < PRECISION) {
                    break;
                }
            }
        }
        rst_map.find(t_id)->second += F;

        cdf.find(t_id)->second += f;
    }

    //free memory
    free(samples);

    return TRUE;
}

int recreate_trajs(vector<OctPoint *> pt_vector, hash_map<IDTYPE, Traj*> &trajs) {
    size_t i;
    hash_map<int, OctPoint *> traj_head; 
    hash_map<int, OctPoint *> traj_tail; 
    hash_map<int, OctPoint *>::iterator it; 
    OctPoint *cur_point,*head, *tail;
    for (i = 0; i < pt_vector.size(); i++) {
        cur_point = pt_vector[i];

        it = traj_head.find(cur_point->p_tid);
        if(it == traj_head.end()) {
            traj_head.insert(pair<int, OctPoint *>(cur_point->p_tid, cur_point));
        } else {
            head = it->second;
            if(cur_point->p_xyz[2] < head->p_xyz[2]) {
                it->second = cur_point;
            }
        }

        it = traj_tail.find(cur_point->p_tid);
        if(it == traj_tail.end()) {
            traj_tail.insert(pair<int, OctPoint *>(cur_point->p_tid, cur_point));
        } else {
            tail = it->second;
            if(cur_point->p_xyz[2] > tail->p_xyz[2]) {
                it->second = cur_point;
            }
        }
    }
    
    hash_map<int, Traj *>::iterator tit;
    for(it = traj_head.begin(); it != traj_head.end(); ++it) {
        head = it->second;
        if(head->pre != -1) {
            head = g_PtList.find(head->pre)->second;
        }
        Traj *t = new Traj(head->p_tid, head->p_id, -1);
        trajs.insert(pair<int, Traj*>(t->t_id, t));
    }

    for(it = traj_tail.begin(); it != traj_tail.end(); ++it) {
        tail = it->second;
        if(tail->next != -1) {
            tail = g_PtList.find(tail->next)->second;
        }
        tit = trajs.find(tail->p_tid);
        if(tit == trajs.end()) {
            write_log(ERROR_LOG, "recreate_trajs: trajs find failed.\n");
            return FALSE;
        }
        Traj *t = tit->second;
        t->t_tail = tail->p_id;
    }

    return TRUE;
}

int package_refinement_data(hash_map<IDTYPE, Traj*> &trajs, \
        struct query_struct query, char **buf_ptr, uint32_t *data_size, \
        struct refinement_stat * r_stat) {

    uint32_t buf_size = SOCKET_BUF_SIZE * 10;

    char *buf = (char *) malloc(buf_size);
    if(buf == NULL) {
        write_log(ERROR_LOG, "package_refinement_data: malloc buf failed.\n");
        return FALSE;
    }

    uint32_t traj_num = trajs.size();
    size_t cpy_len = 0, i;
    size_t pair_num_pos = 0;
    uint32_t pair_num = 0;
    uint32_t segs_num = 0;

    OctPoint *p_cur, *p_next;
    struct point start, end;

    hash_map<IDTYPE, Traj *>::iterator tit;
    for(tit = trajs.begin(); tit != trajs.end(); tit++) {
        Traj *traj = tit->second;
        IDTYPE pid = traj->t_head;

        p_cur =  g_PtList.find(pid)->second;
        pair_num = 0;

        memcpy(buf + cpy_len, &traj->t_id, sizeof(uint32_t));
        cpy_len += sizeof(uint32_t);

        // reserve this space for store pair num
        pair_num_pos = cpy_len;
        cpy_len += sizeof(uint32_t);

        int traj_contained = FALSE;

        while(p_cur->next != -1) {
            pid = p_cur->next; 

            p_next = g_PtList.find(pid)->second; 
            // ignore the interpolation point
            while(p_next->p_id < -1) {
                pid = p_next->p_id; 
                p_next = g_PtList.find(pid)->second; 
            }
            
            //construct a line segment  
            for(i = 0; i < MAX_DIM_NUM; i++) {
                start.axis[i] = p_cur->p_xyz[i];
                end.axis[i] = p_next->p_xyz[i];
            }

            if(1 == line_intersect(query.intval, start, end)){
                traj_contained = TRUE;

                // if current start-end segment contained in the query
                // move buf ptr back to pair_num_pos, so, no need to refine this trajectory. 
                if (query.op == RANGE_QUERY && \
                                line_contain(query.intval, start, end)) {
                    cpy_len = pair_num_pos + sizeof(uint32_t);
                    pair_num = 0;
                    break;
                }

                pair_num++;

                memcpy(buf + cpy_len, &start, sizeof(struct point));
                cpy_len += sizeof(struct point);
                memcpy(buf + cpy_len, &end, sizeof(struct point));
                cpy_len += sizeof(struct point);

                //check if we need to increase the buf size;
                // note that this also ensure we can store the first line segment of the next trajectory
                if(cpy_len + sizeof(struct point) * 2 + sizeof(uint32_t)*2 >= buf_size) {
                    buf_size += SOCKET_BUF_SIZE * 5;
                    char *buf_cpy= (char *)realloc(buf, buf_size);
                    if(buf_cpy == NULL) {
                        // TODO release memory
                        write_log(ERROR_LOG, "package_refinement_data: realloc buf_cpy failed.\n");
                        return FALSE;
                    }
                    buf = buf_cpy; 
                }
            }
            // move next
            p_cur = p_next;
        }//end of processing a traj

        //store the pair_num
        memcpy(buf+pair_num_pos, &pair_num, sizeof(uint32_t));
 
        // increase total num of segments 
        segs_num += pair_num;

        //update the traj_num if necessary
        if (traj_contained == FALSE){
            traj_num--;
            //roll back the cpy_len
            cpy_len = pair_num_pos - sizeof(uint32_t); 
        }

        //check if we need to increase the buf size;
        // note that this also ensure we can store the first line segment of the next trajectory
        if(cpy_len + sizeof(struct point) * 2 + sizeof(uint32_t)*2 >= buf_size) {
            buf_size += SOCKET_BUF_SIZE * 5;
            char *buf_cpy= (char *)realloc(buf, buf_size);
            if(buf_cpy == NULL) {
                // TODO release memory
                write_log(ERROR_LOG, "package_refinement_data: realloc buf_cpy failed.\n");
                return FALSE;
            }
            buf = buf_cpy; 
        }
    }

    *buf_ptr = buf;
    *data_size = cpy_len;

    r_stat->traj_num = traj_num;
    r_stat->segs_num = segs_num;
    r_stat->data_volume = cpy_len;
    
    return TRUE;
}

//partition the original query into num sub_queries on the time dimension
int partition_query(struct query_struct query, struct query_struct sub_queries[], int num){
    int i;
    double low, high, step;
    low = query.intval[2].low;
    high = query.intval[2].high;
    step = (high-low)/num;

    for (i=0;i<num;i++){
        sub_queries[i] = query;
        sub_queries[i].intval[2].low = low + i*step;
        sub_queries[i].intval[2].high = sub_queries[i].intval[2].low + step;
        if (i == num-1){
            sub_queries[i].intval[2].high = high;
        }
    }
    return TRUE;
}
int compute_range_query_stat(hash_map<IDTYPE, Traj*> &trajs, struct query_struct query, struct refinement_stat *r_stat) {
    uint32_t pair_num = 0;
    uint32_t segs_num = 0;
    double total_time_span = 0;
    uint32_t traj_num = trajs.size();

    OctPoint *p_cur, *p_next;
    struct point start, end;
    double traj_start_time, traj_end_time;

    hash_map<IDTYPE, Traj *>::iterator tit;
    for(tit = trajs.begin(); tit != trajs.end(); tit++) {
        Traj *traj = tit->second;
        IDTYPE pid = traj->t_head;

        p_cur =  g_PtList.find(pid)->second;
        traj_start_time = p_cur->p_xyz[2];
        pair_num = 0;

        while(p_cur->next != -1) {
            pid = p_cur->next; 
            p_next = g_PtList.find(pid)->second; 

            // ignore the interpolation point
            while(p_next->p_id < -1) {
                pid = p_next->p_id; 
                p_next = g_PtList.find(pid)->second; 
            }
            traj_end_time = p_next->p_xyz[2];
            
            //construct a line segment  
            int i;
            for(i = 0; i < MAX_DIM_NUM; i++) {
                start.axis[i] = p_cur->p_xyz[i];
                end.axis[i] = p_next->p_xyz[i];
            }

            if(1 == line_intersect(query.intval, start, end)){
                if(query.op == RANGE_QUERY && 1 == line_contain(query.intval, start, end)) {
                    pair_num = 0;
                    break;
                }
                pair_num++;
            }
            // move next
            p_cur = p_next;
        }
        // increase total num of segments 
        segs_num += pair_num;

        // calc total_time_span
        if(pair_num > 0) {
            total_time_span += (traj_end_time - traj_start_time);
        }
    }

    r_stat->traj_num = traj_num;
    r_stat->segs_num = segs_num;
    if(traj_num > 0) {
        r_stat->avg_time_span = total_time_span / traj_num;
    } else {
        r_stat->avg_time_span = 0;
    }
    r_stat->data_volume = segs_num * sizeof(point) * 2 + traj_num * sizeof(uint32_t)*2;

    return TRUE;
}
    
int send_refinement_request(hash_map<IDTYPE, Traj*> &trajs, struct query_struct query, char *dst_ip){
    uint32_t data_size;
    char *data = NULL;
    struct refinement_stat r_stat;

    package_refinement_data(trajs, query, &data, &data_size, &r_stat);

    // create a data socket to send refinement data
	int socketfd;
	socketfd = new_client_socket(dst_ip, COMPUTE_WORKER_PORT);
	if (FALSE == socketfd) {
        write_log(TORUS_NODE_LOG, "send_refinement_request: create socket failed\n");
		return FALSE;
	}

	// get local ip address
	char local_ip[IP_ADDR_LENGTH];
	memset(local_ip, 0, IP_ADDR_LENGTH);
	if (FALSE == get_local_ip(local_ip)) {
	    //TODO Jinchuan write err log
 	    return FALSE;
	}

    size_t len = 0;
    struct message msg;
    if (query.op == RANGE_QUERY){
        msg.op = RANGE_QUERY_REFINEMENT;
    }else if (query.op == RANGE_NN_QUERY){
        msg.op = NN_QUERY_REFINEMENT;
    }
    strncpy(msg.src_ip, local_ip, IP_ADDR_LENGTH);
    strncpy(msg.dst_ip, dst_ip, IP_ADDR_LENGTH);
    strncpy(msg.stamp, "", STAMP_SIZE);

    memcpy(msg.data, &query, sizeof(query_struct));
    len += sizeof(query_struct);
    memcpy(msg.data + len, &r_stat, sizeof(refinement_stat));
    len += sizeof(refinement_stat);
    memcpy(msg.data + len, &data_size, sizeof(uint32_t));
    len += sizeof(uint32_t);
    send_safe(socketfd, (void *)&msg, sizeof(struct message), 0);

    // begin to send points to idle_ip
    send_safe(socketfd, (void *)data, data_size, 0);
    // release the buf's space
    free(data);

	close(socketfd);
    return TRUE;
}

int localize_query(struct query_struct *query) {
    int i;
    for(i = 0; i < MAX_DIM_NUM; i++) {
        if(query->intval[i].low < the_torus->info.region[i].low) {
            query->intval[i].low = the_torus->info.region[i].low;
        }
        if(query->intval[i].high > the_torus->info.region[i].high) {
            query->intval[i].high = the_torus->info.region[i].high;
        }
    }
    return TRUE;
}

int send_throw_out_query_msg(struct query_struct query, struct refinement_stat overall_stat) {
    struct message notify_msg;
    notify_msg.op = NOTIFY_MESSAGE;
    strncpy(notify_msg.src_ip, the_torus->info.ip, IP_ADDR_LENGTH);
    strncpy(notify_msg.dst_ip, result_ip, IP_ADDR_LENGTH);
    strncpy(notify_msg.stamp, "", STAMP_SIZE);

    char note[200];
    memset(note, 0, 200);
    sprintf(note, "the query %d is very big and be thrown out. %u %u", \
            query.data_id, overall_stat.traj_num, overall_stat.segs_num);
    memcpy(notify_msg.data, &note, strlen(note));
    send_safe(result_sockfd, (void *) &notify_msg, sizeof(struct message), 0);
    return TRUE;
}


int local_oct_tree_query(struct query_struct query, double low[], double high[]) {
    int i;
    struct timespec begin, end;
    double elapsed = 0L;
    struct filter_log_struct fl_st;

    //char buffer[1024];
    clock_gettime(CLOCK_REALTIME, &begin);

    //step 0: localize query
    localize_query(&query);

    fl_st.query_id = query.data_id;
    memcpy(fl_st.qry_region, query.intval, sizeof(struct interval) * MAX_DIM_NUM);

    //step 1: query trajs which overlap with region(low, high)
    vector<OctPoint*> pt_vector;

    // local range query
    the_torus_oct_tree->rangeQuery(low, high, pt_vector);

    // recreate all trajs with pt_vector(range query result)
    hash_map<IDTYPE, Traj*> trajs;
    recreate_trajs(pt_vector, trajs);

    clock_gettime(CLOCK_REALTIME, &end);
    elapsed = get_elasped_time(begin, end) / 1000000.0;
    fl_st.index_elapsed = elapsed;
    fl_st.size_after_index = trajs.size();
    
    //step 2: compute statistics and estimate the num of idle nodes
    int expect_response = 0, requested_num = 0, actual_got_num = 0;
    struct refinement_stat overall_stat;
    compute_range_query_stat(trajs, query, &overall_stat);

    clock_gettime(CLOCK_REALTIME, &begin);
    // if the num of fixed idle node is 0, 
    // this means to estimate response and requested node num
    if(FIXED_IDLE_NODE_NUM != 0) {
        requested_num = FIXED_IDLE_NODE_NUM;
    } else {
        expect_response = estimate_response_time(overall_stat, query);
        requested_num = (expect_response / MAX_RESPONSE_TIME) + 1;
    }

    //step 3:try to find requested_num idle torus nodes to do refinement
    // return actual_got_num idle torus nodes, if actual_got_num is 0,
    // this means the whole torus is very busy, this query refinement will
    // do itself
    char idle_ip[requested_num][IP_ADDR_LENGTH];
    memset(idle_ip, 0, requested_num * IP_ADDR_LENGTH);
    find_idle_torus_node(idle_ip,requested_num,&actual_got_num);
    if (actual_got_num == 0) {
        // this means the whole torus is busy now
        send_throw_out_query_msg(query, overall_stat);
        //strncpy(idle_ip[0], the_torus->info.ip, IP_ADDR_LENGTH);
        //actual_got_num = 1;
        return TRUE;
    }
    fl_st.requested_num = requested_num;
    fl_st.actual_got_num = actual_got_num;

    clock_gettime(CLOCK_REALTIME, &end);
    elapsed = get_elasped_time(begin, end) / 1000000.0;
    fl_st.obtain_idle_elapsed = elapsed;

    //step 3.1: test whether we need to throw out this query 
    // when we estimate the query is very big
    // the time for estimated send refinement and do refinement
    int expect_process_time = 0;
    //time to package and send refinement data
    expect_process_time += overall_stat.segs_num * EXCHANGE_RATE_PACKAGE_DATA; 
    //time to do refinement
    expect_process_time += expect_response / actual_got_num; 
    if (expect_process_time >= TOLERABLE_RESPONSE_TIME){
        //send notify msg and return
        send_throw_out_query_msg(query, overall_stat);
        return TRUE;
    }


    //step 4: partition query into sub_queries
    clock_gettime(CLOCK_REALTIME, &begin);
    struct query_struct sub_queries[actual_got_num];
    partition_query(query, sub_queries, actual_got_num);

    //step 5: send_refinement_request to actual_got_num idle nodes
    for(i = 0; i < actual_got_num; i++) {
        send_refinement_request(trajs, sub_queries[i], idle_ip[i]);
    }

    clock_gettime(CLOCK_REALTIME, &end);
    elapsed = get_elasped_time(begin, end) / 1000000.0;
    fl_st.send_refinement_elapsed = elapsed;
    //TODO should calculate the size after filter
    fl_st.size_after_filter = 0;

    // free trajs hash_map
    hash_map<IDTYPE, Traj*>::iterator tit;
    for(tit = trajs.begin(); tit != trajs.end(); tit++) {
        Traj *traj = tit->second;
        delete traj;
    }
    trajs.clear();

    //send statistic to result node
    struct message msg;
    msg.op = RECEIVE_FILTER_LOG; 
    strncpy(msg.src_ip, the_torus->info.ip, IP_ADDR_LENGTH);
    strncpy(msg.dst_ip, result_ip, IP_ADDR_LENGTH);
    strncpy(msg.stamp, "", STAMP_SIZE);
    
    memcpy(msg.data, &fl_st, sizeof(struct filter_log_struct));

    send_safe(result_sockfd, (void *) &msg, sizeof(struct message), 0);

    return TRUE;
}

int estimate_response_time(struct refinement_stat r_stat, struct query_struct query) {
    int res_time = 0;
    int num = SAMPLE_SPATIAL_POINTS * SAMPLE_TIME_POINTS; 
    num *= r_stat.segs_num;
    if (query.op == RANGE_QUERY){
        res_time = (int) (EXCHANGE_RATE_RANGE_QUERY * num);

    } else if (query.op == RANGE_NN_QUERY){
        double d_max = (query.intval[0].low + query.intval[0].high) / 2.0;
        double qry_width = query.intval[2].high - query.intval[2].low;
        double density = r_stat.traj_num * r_stat.avg_time_span / qry_width;
        double rho = density / (4.0 * d_max * d_max);
        double tmp = sqrt(-((log10(1 - 0.7) / log10(2.718281))/ (PI * rho)));

        double num_snapshot_qrys = qry_width / SAMPLE_TIME_RATE;
        double sample_points_per_time = rho * PI * pow(tmp + 3 * SIGMA, 2.0);

        res_time = (int)(EXCHANGE_RATE_NN_QUERY * num_snapshot_qrys * pow(sample_points_per_time, 2.0));
    }
    return res_time;
}

double calc_QP(struct interval region[], point start, point end, int n, int m, point **samples) {
    int cnt, i, j;
    double value, z;
    // center_point
    struct point cpt;

    // init gsl lib
    gsl_rng *r;
    gsl_rng_env_setup();
    gsl_rng_default_seed = ((unsigned long)(time(NULL)));
    r = gsl_rng_alloc(gsl_rng_default);

    for(i = 0, cnt = 0; cnt < n; cnt++) {
        // sample z(time) axis with uniform distribution
        value = gsl_ran_flat(r, start.axis[2], end.axis[2]);

        // sample x, y axis with gaussian distribution
        /* given points start and end and sample point p's z axis,
         * note that these three points are in one line.
         * calc samples point's x, y method:
         *  (p.x[y] - start.x[y])     (end.x[y] - start.x[y]) 
         * ----------------------- = -------------------------
         *    (p.z - start.z)              (end.z - p.z) 
         * */
        z = value;
        cpt.axis[0] = ((z - start.axis[2]) * end.axis[0] + (end.axis[2] - z) * start.axis[0]) / (end.axis[2] - start.axis[2]);
        cpt.axis[1] = ((z - start.axis[2]) * end.axis[1] + (end.axis[2] - z) * start.axis[1]) / (end.axis[2] - start.axis[2]);
        cpt.axis[2] = z;

        // if one of n flat distribution sample point not contianed in region, 
        // skip this sample point
        if(1 == point_contain(cpt, region)) {
            for(j = 0; j < m; j++) {
                samples[i][j].axis[2] = cpt.axis[2];
            }

            j = 0;
            while(j < m) {
                value = gsl_ran_gaussian(r, SIGMA); 
                if(value < -3 * SIGMA || value > 3 * SIGMA) {
                    continue;
                }
                samples[i][j].axis[0] = cpt.axis[0] + value;
                j++;
            }

            j = 0;
            while(j < m) {
                value = gsl_ran_gaussian(r, SIGMA); 
                if(value < -3 * SIGMA || value > 3 * SIGMA) {
                    continue;
                }
                samples[i][j].axis[1] = cpt.axis[1] + value;
                j++;
            }
            i++;
        }
    }
    gsl_rng_free(r);

    // i is the final flat sample points
    int flat_cnt = i;
    // calculate QP
    double qp = 0.0, F[n], MF = 1;
    // the probability of each time samples(n)
    double pt = 1.0 / m;
    for(i = 0; i < flat_cnt; i++) {
        F[i] = 0.0;
        for(j = 0; j < m; j++) {
            if(1 == point_contain(samples[i][j], region)) {
                F[i] += pt;
            }
        }
        MF *= (1 - F[i]);
    }
    qp = 1 - MF;
    return qp;
}

/*double calc_QP(struct interval region[], int n, int m, point **samples) {
    //struct interval intval[MAX_DIM_NUM];
    int i, j;
    double qp = 0.0, F[n], MF = 1;
    // the probability of each time samples(n)
    double pt = 1.0 / m;
    //char buf[1024];

    for(i = 0; i < n; i++) {
        F[i] = 0.0;
        for(j = 0; j < m; j++) {
            if(1 == point_contain(samples[i][j], region)) {
                F[i] += pt;
            }
        }
        MF *= (1 - F[i]);
        // check MF < 1 - theta
        if( MF < 1 - REFINEMENT_THRESHOLD) {
            return (1 - MF);
        }
    }
    qp = 1 - MF;
    return qp;
}*/

double calc_refinement(struct interval region[], point start, point end) {
    // the time sampling size n 
    // and other two sampling size m
    int i, j, k, n = SAMPLE_TIME_POINTS, m = SAMPLE_SPATIAL_POINTS;
    struct point **samples;
    samples = (point **)malloc(sizeof(point*) * n);
    for(i = 0; i < n; i++) {
        samples[i] = (point *)malloc(sizeof(point) * m);
        for(j = 0; j < m; j++) {
            for(k = 0; k < MAX_DIM_NUM; k++) {
                samples[i][j].axis[k] = 0;
            }
        }
    } 

    // generate n*m samples points
    double qp = calc_QP(region, start, end, n, m, samples);

    // release the samples mem
    for(i = 0; i < n; i++){
        free(samples[i]);
    }
    free(samples);

    return qp;
}

int operate_oct_tree(struct query_struct query, int hops) {

    if(the_torus_oct_tree == NULL) {
        write_log(ERROR_LOG, "torus oct tree didn't create.\n");
        return FALSE;
    }

    int i;
    double plow[MAX_DIM_NUM], phigh[MAX_DIM_NUM];

    for (i = 0; i < MAX_DIM_NUM; i++) {
        plow[i] = (double)query.intval[i].low;
        phigh[i] = (double)query.intval[i].high;
    }

    // rtree operation 
    if(query.op == DATA_INSERT) {
        pthread_mutex_lock(&mutex);
		OctPoint *point = new OctPoint(query.data_id, -1, plow, query.trajectory_id, NONE, NONE);//后继指针都为空
		if(the_torus_oct_tree->containPoint(point)){
            timespec start, end;
            clock_gettime(CLOCK_REALTIME, &start);
            oct_tree_insert(point);
            if(g_NodeList.find(1)->second->n_ptCount >= (int)the_torus->info.capacity) {
                torus_split();
            }
            clock_gettime(CLOCK_REALTIME, &end);
            static double elasped = 0L;
            elasped += (get_elasped_time(start, end) / 1000000.0);

            static int count = 0;
            static long hops_sum = 0;
            count++;
            if(count % 100000 == 0) {
                /*char b[20];
                sprintf(b, "insert:%d\n", count);
                write_log(TORUS_NODE_LOG, b);*/
                struct message msg;
                msg.op = THROUGHPUT_TEST;
                strncpy(msg.src_ip, the_torus->info.ip, IP_ADDR_LENGTH);
                strncpy(msg.dst_ip, result_ip, IP_ADDR_LENGTH);
                strncpy(msg.stamp, "", STAMP_SIZE);
                int c = 100000;
                double hop = hops_sum / (100000 * 1.0);
                double el = elasped / (100000 * 1.0);
                hops_sum = 0;
                elasped = 0;

                int cpy_len = 0;
                memcpy(msg.data, &c, sizeof(int));
                cpy_len += sizeof(int);
                memcpy(msg.data + cpy_len, &hop, sizeof(double));
                cpy_len += sizeof(double);
                memcpy(msg.data + cpy_len, &el, sizeof(double));
                cpy_len += sizeof(double);
                memcpy(msg.data + cpy_len, &the_torus->info.node_id, sizeof(coordinate));
                send_safe(result_sockfd, (void *) &msg, sizeof(struct message), 0);
            }
            hops_sum += hops;
		}
        pthread_mutex_unlock(&mutex);
    } else if(query.op == RANGE_NN_QUERY) {
        pthread_mutex_lock(&mutex);
        //write_log(TORUS_NODE_LOG, "torus node local nn query\n");
        //local_oct_tree_nn_query(query.intval, plow, phigh);
        local_oct_tree_query(query, plow, phigh);
        pthread_mutex_unlock(&mutex);
    } else if(query.op == RANGE_QUERY) {
        pthread_mutex_lock(&mutex);
        //write_log(TORUS_NODE_LOG, "torus node local range query\n");
        local_oct_tree_query(query, plow, phigh);
        pthread_mutex_unlock(&mutex);
    }
    return TRUE;
}

int do_rtree_load_data(connection_t conn, struct message msg){
    struct timespec start, end;
    double elasped = 0L;

    clock_gettime(CLOCK_REALTIME, &start);

    byte *data_ptr, *ptr;
    id_type data_id;
    uint32_t package_num, data_len;// package_len;
    Region region;
    static int count = 0;

    int length = 0, loop = 1;
    std::vector<SpatialIndex::IData *> v;

    byte buf[SOCKET_BUF_SIZE];
    memset(buf, 0, SOCKET_BUF_SIZE);

    while(loop) {
        length = recv_safe(conn->socketfd, buf, SOCKET_BUF_SIZE, 0);
        if(length == 0) {
            // client nomally closed
            close_connection(conn);
            loop = 0;
        } else {
            length = 0;
            ptr = reinterpret_cast<byte *>(buf); 
            memcpy(&package_num, ptr, sizeof(uint32_t));
            ptr += sizeof(uint32_t);
            count += package_num;
            while(package_num--){

                //memcpy(&package_len, ptr, sizeof(uint32_t));
                //ptr += sizeof(uint32_t);

                memcpy(&data_id, ptr, sizeof(id_type));
                ptr += sizeof(id_type);

                memcpy(&data_len, ptr , sizeof(uint32_t));
                ptr += sizeof(uint32_t);

                if (data_len > 0){
                    data_ptr = new byte[data_len];
                    memcpy(data_ptr, ptr, data_len);
                    ptr += data_len;
                }

                region.loadFromByteArray(ptr);
                ptr += region.getByteArraySize();

                SpatialIndex::IData *data = dynamic_cast<SpatialIndex::IData*>(new Data(data_len, data_ptr, region, data_id));
                v.push_back(data);
            }
            memset(buf, 0, SOCKET_BUF_SIZE);
        } 
    }

    clock_gettime(CLOCK_REALTIME, &end);
    elasped = get_elasped_time(start, end);

    char buffer[1024];
    memset(buffer, 0, 1024);
    sprintf(buffer, "receive split data %f ms\n", (double) elasped/ 1000000.0);
    write_log(RESULT_LOG, buffer);

    //bulkload rtree
    the_torus_rtree = rtree_bulkload(v);

    std::vector<SpatialIndex::IData*>::iterator it;
    for (it = v.begin(); it != v.end(); it++) {
        delete *it;
    } 

    size_t c = rtree_get_utilization(the_torus_rtree);
    memset(buffer, 0, 1024);
    sprintf(buffer, "%lu\n", c);
    write_log(RESULT_LOG, buffer);

    time_t now;
    now = time(NULL);
    memset(buffer, 0, 1024);
    sprintf(buffer, "%ld %d\n", now, count);
    write_log(RESULT_LOG, buffer);
    return TRUE;
}

int do_nn_query_refinement(connection_t conn, struct message msg) {
    struct timespec begin, end;
    double elapsed = 0L;
    struct refinement_log_struct rfmt_st;

    clock_gettime(CLOCK_REALTIME, &begin);

    //step 1: parse refinement head from message
    struct query_struct query;
    struct refinement_stat r_stat;
    uint32_t data_size = 0;
    parse_refinement_head(msg, &r_stat, &query, &data_size);

    // increase current torus node's stat (fvalue)
    update_max_fvalue(r_stat, query, 1);

    rfmt_st.query_id = query.data_id;
    memcpy(rfmt_st.qry_region, query.intval, sizeof(struct interval) * MAX_DIM_NUM);

    //step 2: receive refinement data from conn
    uint32_t traj_num;
    traj_num = r_stat.traj_num;
    struct traj_segments traj_segs[traj_num];
    receive_refinement_data(conn, data_size, traj_segs, traj_num);

    rfmt_st.result_size = traj_num;

    clock_gettime(CLOCK_REALTIME, &end);
    elapsed = get_elasped_time(begin, end) / 1000000.0;
    rfmt_st.recv_refinement_elapsed = elapsed;


    // step 3: nn query refinement
    clock_gettime(CLOCK_REALTIME, &begin);
    double avg_qp;
    calc_nn_query_refinement(query,traj_segs,traj_num, &avg_qp);
    clock_gettime(CLOCK_REALTIME, &end);
    elapsed = get_elasped_time(begin, end) / 1000000.0;
    rfmt_st.calc_qp_elapsed = elapsed;
    rfmt_st.avg_qp = avg_qp;

    update_max_fvalue(r_stat, query, -1);

    // send range query refinement result to result ip
    struct message res_msg;
    res_msg.op = RECEIVE_REFINEMENT_LOG;
    strncpy(res_msg.src_ip, the_torus->info.ip, IP_ADDR_LENGTH);
    strncpy(res_msg.dst_ip, result_ip, IP_ADDR_LENGTH);
    strncpy(res_msg.stamp, "", STAMP_SIZE);
    memcpy(res_msg.data, &rfmt_st, sizeof(struct refinement_log_struct));

    send_safe(result_sockfd, (void *) &res_msg, sizeof(struct message), 0);

    return TRUE;
}

int parse_refinement_head(struct message msg, struct refinement_stat *r_stat, struct query_struct *query, uint32_t *data_size) {
    size_t len = 0;
    // parse message 
    memcpy(query, msg.data, sizeof(query_struct));
    len += sizeof(query_struct);

    memcpy(r_stat, msg.data + len, sizeof(refinement_stat));
    len += sizeof(refinement_stat);

    memcpy(data_size, msg.data + len, sizeof(uint32_t));
    len += sizeof(uint32_t);

    return TRUE;
}
int receive_refinement_data(connection_t conn, uint32_t data_size, struct traj_segments traj_segs[], uint32_t traj_num) {
    uint32_t i, pair_num;
    for(i = 0; i < traj_num; i++) {
        traj_segs[i].t_id = -1;
        traj_segs[i].pair_num = 0;
        traj_segs[i].start = NULL;
        traj_segs[i].end = NULL;
    }

    int length = 0, loop = 1;
    char buf[data_size];
    memset(buf, 0, data_size);

    while(loop){
        length = recv_safe(conn->socketfd, buf, data_size, 0);
        if(length <= 0) {
            close_connection(conn);
            loop = 0;
        } else {
            int traj_idx = 0;
            uint32_t len = 0;

            while(1){
                memcpy(&traj_segs[traj_idx].t_id, buf + len, sizeof(uint32_t));
                len += sizeof(uint32_t);
                memcpy(&traj_segs[traj_idx].pair_num, buf + len, sizeof(uint32_t));
                len += sizeof(uint32_t);

                // allocate space to store current traj's segments
                pair_num = traj_segs[traj_idx].pair_num;
                traj_segs[traj_idx].start = (point *) malloc(sizeof(struct point) * pair_num);
                traj_segs[traj_idx].end = (point *) malloc(sizeof(struct point) * pair_num);

                // read segments of current trajectory from buf
                for(i = 0; i < pair_num; i++) {
                    memcpy(&traj_segs[traj_idx].start[i], buf + len, sizeof(struct point));
                    len += sizeof(struct point);
                    memcpy(&traj_segs[traj_idx].end[i], buf + len, sizeof(struct point));
                    len += sizeof(struct point);
                }
                
                // read finish
                if(len >= data_size) {
                    break;
                }
                traj_idx++;
            }
        }

    }
    return TRUE;
}

int do_range_query_refinement(connection_t conn, struct message msg) {
    struct timespec begin, end;
    double elapsed = 0L;
    struct refinement_log_struct rfmt_st;

    clock_gettime(CLOCK_REALTIME, &begin);

    //step 1: parse refinement head from message
    struct query_struct query;
    struct refinement_stat r_stat;
    uint32_t data_size = 0;
    parse_refinement_head(msg, &r_stat, &query, &data_size);

    // increase current torus node's stat (fvalue)
    update_max_fvalue(r_stat, query, 1);

    rfmt_st.query_id = query.data_id; 
    memcpy(rfmt_st.qry_region,query.intval, sizeof(struct interval) * MAX_DIM_NUM);  

    //step 2: receive refinement data from conn
    uint32_t traj_num;
    traj_num = r_stat.traj_num;
    struct traj_segments traj_segs[traj_num];
    receive_refinement_data(conn, data_size, traj_segs, traj_num);

    rfmt_st.result_size = traj_num;

    clock_gettime(CLOCK_REALTIME, &end);
    elapsed = get_elasped_time(begin, end) / 1000000.0;
    rfmt_st.recv_refinement_elapsed = elapsed; 

    clock_gettime(CLOCK_REALTIME, &begin);
    //step 3: refinement
    uint32_t i, j = 0;
    double total_qp = 0.0;

    for(i = 0; i < traj_num; i++) {
        double traj_qp = 1.0;
        if(traj_segs[i].t_id != 0) {
            if(traj_segs[i].pair_num == 0) {
                continue;
            } else { 
                for(j = 0; j < traj_segs[i].pair_num; j++) {
                   traj_qp *= (1.0 - calc_refinement(query.intval, traj_segs[i].start[j], traj_segs[i].end[j]));
                }
            }

            // release the memory
            if(traj_segs[i].start != NULL) {
                free(traj_segs[i].start);
            }
            if(traj_segs[i].end != NULL) {
                free(traj_segs[i].end);
            }
        }
        traj_qp = 1.0 - traj_qp; 
        total_qp += traj_qp; 
    }

    if(traj_num == 0 ){
        rfmt_st.avg_qp = 0;
    }else{
        rfmt_st.avg_qp = total_qp/ traj_num;
    }

    clock_gettime(CLOCK_REALTIME, &end);
    elapsed = get_elasped_time(begin, end) / 1000000.0;
    rfmt_st.calc_qp_elapsed = elapsed; 

    // decrease current fvalue
    update_max_fvalue(r_stat, query, -1);

    // send range query refinement result to result ip
    struct message res_msg;
    res_msg.op = RECEIVE_REFINEMENT_LOG;
    strncpy(res_msg.src_ip, the_torus->info.ip, IP_ADDR_LENGTH);
    strncpy(res_msg.dst_ip, result_ip, IP_ADDR_LENGTH);
    strncpy(res_msg.stamp, "", STAMP_SIZE);
    memcpy(res_msg.data, &rfmt_st, sizeof(struct refinement_log_struct));

    send_safe(result_sockfd, (void *) &res_msg, sizeof(struct message), 0);

    return TRUE;
}

int do_load_oct_tree_points(connection_t conn, struct message msg) {
    struct timespec start, end;
    double elasped = 0L;
    clock_gettime(CLOCK_REALTIME, &start);

    uint32_t package_num;
    int length = 0, loop = 1;
    char buf[SOCKET_BUF_SIZE], *ptr;
    memset(buf, 0, SOCKET_BUF_SIZE);

    int min_p_id = -1;

    while(loop) {
        length = recv_safe(conn->socketfd, buf, SOCKET_BUF_SIZE, 0);
        if(length == 0) {
            // client nomally closed
            close_connection(conn);
            loop = 0;
        } else {
            length = 0;
            ptr = buf; 
            memcpy(&package_num, ptr, sizeof(uint32_t));
            ptr += sizeof(uint32_t);
            while(package_num--){
                OctPoint *point = new OctPoint();
                point->loadFromByteArray(ptr);
                ptr += point->getByteArraySize();
                g_PtList.insert(pair<IDTYPE, OctPoint*>(point->p_id, point));
                if( min_p_id > point->p_id) {
                    min_p_id = point->p_id;
                }
            }
            memset(buf, 0, SOCKET_BUF_SIZE);
        }
    }

    clock_gettime(CLOCK_REALTIME, &end);
    elasped = get_elasped_time(start, end);

    g_ptNewCount = -(min_p_id + 1);

    char buffer[1024];
    memset(buffer, 0, 1024);
    sprintf(buffer, "receive split oct tree points spend %f ms\n", (double) elasped/ 1000000.0);
    write_log(RESULT_LOG, buffer);

    //printPoints();

    return TRUE;
}

int do_load_oct_tree_nodes(connection_t conn, struct message msg) {
    struct timespec start, end;
    double elasped = 0L;
    clock_gettime(CLOCK_REALTIME, &start);

    uint32_t package_num;
    int length = 0, loop = 1, type = -1, node_id = -1, max_n_id = -1;
    char buf[SOCKET_BUF_SIZE], *ptr;
    memset(buf, 0, SOCKET_BUF_SIZE);

    while(loop) {
        length = recv_safe(conn->socketfd, buf, SOCKET_BUF_SIZE, 0);
        if(length == 0) {
            // client nomally closed
            close_connection(conn);
            loop = 0;
        } else {
            length = 0;
            ptr = buf; 
            memcpy(&package_num, ptr, sizeof(uint32_t));
            ptr += sizeof(uint32_t);
            while(package_num--){
                memcpy(&type, ptr, sizeof(int));

                if( type == LEAF) {
                    OctLeafNode *leaf = new OctLeafNode();
                    leaf->loadFromByteArray(ptr);
                    ptr += leaf->getByteArraySize();

                    node_id = leaf->n_id;
                    g_NodeList.insert(pair<int, OctTNode *>(node_id, leaf));
                } else if(type == IDX) {
                    OctIdxNode *idx = new OctIdxNode();
                    idx->loadFromByteArray(ptr);
                    ptr += idx->getByteArraySize();

                    node_id = idx->n_id;
                    g_NodeList.insert(pair<int, OctTNode *>(node_id, idx));
                }

                if( node_id > max_n_id) {
                    max_n_id = node_id;
                }
            }
            memset(buf, 0, SOCKET_BUF_SIZE);
        }
    }

    g_nodeCount = max_n_id + 1;
    g_ptCount = g_NodeList.find(1)->second->n_ptCount;

    clock_gettime(CLOCK_REALTIME, &end);
    elasped = get_elasped_time(start, end);

    char buffer[1024];
    memset(buffer, 0, 1024);
    sprintf(buffer, "receive split oct tree nodes spend %f ms\n", (double) elasped/ 1000000.0);
    write_log(RESULT_LOG, buffer);

    //printNodes();

    return TRUE;
}

int do_load_oct_tree_trajectorys(connection_t conn, struct message msg) {
    struct timespec start, end;
    double elasped = 0L;
    clock_gettime(CLOCK_REALTIME, &start);

    uint32_t package_num;
    int length = 0, loop = 1;
    char buf[SOCKET_BUF_SIZE], *ptr;
    memset(buf, 0, SOCKET_BUF_SIZE);

    while(loop) {
        length = recv_safe(conn->socketfd, buf, SOCKET_BUF_SIZE, 0);
        if(length == 0) {
            // client nomally closed
            close_connection(conn);
            loop = 0;
        } else {
            length = 0;
            ptr = buf; 
            memcpy(&package_num, ptr, sizeof(uint32_t));
            ptr += sizeof(uint32_t);
            while(package_num--){
                Traj *traj= new Traj();
                traj->loadFromByteArray(ptr);
                ptr += traj->getByteArraySize();
                g_TrajList.insert(pair<IDTYPE, Traj *>(traj->t_id, traj));
            }
            memset(buf, 0, SOCKET_BUF_SIZE);
        }
    }

    clock_gettime(CLOCK_REALTIME, &end);
    elasped = get_elasped_time(start, end);

    char buffer[1024];
    memset(buffer, 0, 1024);
    sprintf(buffer, "receive split oct tree trajectorys spend %f ms\n", (double) elasped/ 1000000.0);
    write_log(RESULT_LOG, buffer);

    //printTrajs();

    return TRUE;
}

int do_query_torus_node(struct message msg) {
	int i;
    //char src_ip[IP_ADDR_LENGTH], dst_ip[IP_ADDR_LENGTH];
	char stamp[STAMP_SIZE];

    // get query from message
    // get requst stamp from msg
	strncpy(stamp, msg.stamp, STAMP_SIZE);

    size_t cpy_len = 0;
    int hops;

    // get forward hops from msg
	memcpy(&hops, msg.data, sizeof(int));
    cpy_len += sizeof(int);
	hops++;
	memcpy(msg.data, &hops, sizeof(int));

    // get query string from msg
    struct query_struct query;
    memcpy(&query, msg.data + cpy_len, sizeof(struct query_struct));

	request *req_ptr = find_request(req_list, stamp);

    //struct message new_msg;
	if (NULL == req_ptr) {
        //TODO how can I delete the shit method to judge whether 
        //the request has been handled or not
        if(query.op == RANGE_NN_QUERY || query.op == RANGE_QUERY) {
            req_ptr = insert_request(req_list, stamp);
        }

		if (1 == overlaps(query.intval, the_torus->info.region)) {

            #ifdef WRITE_LOG
                write_log(TORUS_NODE_LOG, "search torus success!\n\t");
                print_node_info(the_torus->info);

                char buf[1024];
                int len = 0;
                memset(buf, 0, 1024);
                len = sprintf(buf, "query:");
                for (i = 0; i < MAX_DIM_NUM; ++i) {
                    #ifdef INT_DATA
                        len += sprintf(buf + len, "[%d, %d] ", query.intval[i].low,
                                query.intval[i].high);
                    #else
                        len += sprintf(buf + len, "[%lf, %lf] ", query.intval[i].low,
                                query.intval[i].high);
                    #endif
                }
                sprintf(buf + len, "\n");
                write_log(CTRL_NODE_LOG, buf);

                len = 0;
                len = sprintf(buf, "%s:", the_torus->info.ip);
                for (i = 0; i < MAX_DIM_NUM; ++i) {
                    #ifdef INT_DATA
                        len += sprintf(buf + len, "[%d, %d] ",
                                the_torus->info.region[i].low,
                                the_torus->info.region[i].high);
                    #else
                        len += sprintf(buf + len, "[%lf, %lf] ",
                                the_torus->info.region[i].low,
                                the_torus->info.region[i].high);
                    #endif
                }
                sprintf(buf + len, "\n%s:search torus success. %d\n\n", msg.dst_ip, hops);
                write_log(CTRL_NODE_LOG, buf);
            #endif

            if(query.op == RANGE_QUERY || query.op == RANGE_NN_QUERY) {
                for (i = 0; i < MAX_DIM_NUM; i++) {
                    forward_search(query.op, query.intval, msg, i);
                }
            }

			// do rtree operation 
            //operate_rtree(query);
            
			// do oct tree operation 
            //write_log(TORUS_NODE_LOG, "operate oct tree\n");
            operate_oct_tree(query, hops);

		} else {
            //if(query.op == RANGE_QUERY) {
                for (i = 0; i < MAX_DIM_NUM; i++) {
                    if (interval_overlap(query.intval[i], the_torus->info.region[i]) != 0) {
                        forward_search(query.op, query.intval, msg, i);
                        break;
                    }
                }
            //}
		}
	}
	/*else {
	 req_ptr->receive_num--;
	 if (req_ptr->receive_num == 0) {
	 remove_request(req_list, stamp);
	}*/
	return TRUE;
}

int write_global_properties() {
    char buf[1024];
    memset(buf, 0, 1024);
    int len = 0;
    len += sprintf(buf + len, "DEFAULT_CAPACITY=%u\n", DEFAULT_CAPACITY);
    len += sprintf(buf + len, "HEARTBEAT_INTERVAL=%d\n", HEARTBEAT_INTERVAL);
    len += sprintf(buf + len, "MAX_ROUTE_STEP=%d\n", MAX_ROUTE_STEP);
    len += sprintf(buf + len, "SIGMA=%lf\n", SIGMA);
    len += sprintf(buf + len, "SAMPLE_TIME_POINTS=%d\n", SAMPLE_TIME_POINTS);
    len += sprintf(buf + len, "SAMPLE_SPATIAL_POINTS=%d\n", SAMPLE_SPATIAL_POINTS);
    len += sprintf(buf + len, "SAMPLE_TIME_RATE=%lf\n", SAMPLE_TIME_RATE);
    len += sprintf(buf + len, "MAX_RESPONSE_TIME=%d\n", MAX_RESPONSE_TIME);
    len += sprintf(buf + len, "TOLERABLE_RESPONSE_TIME=%d\n", TOLERABLE_RESPONSE_TIME);
    len += sprintf(buf + len, "EXCHANGE_RATE_RANGE_QUERY=%lf\n", EXCHANGE_RATE_RANGE_QUERY);
    len += sprintf(buf + len, "EXCHANGE_RATE_NN_QUERY=%lf\n", EXCHANGE_RATE_NN_QUERY);
    len += sprintf(buf + len, "EXCHANGE_RATE_PACKAGE_DATA=%lf\n", EXCHANGE_RATE_PACKAGE_DATA);
    len += sprintf(buf + len, "ACTIVE_LEADER_NUM=%d\n", ACTIVE_LEADER_NUM);
    len += sprintf(buf + len, "FIXED_IDLE_NODE_NUM=%d\n", FIXED_IDLE_NODE_NUM);
    write_log(TORUS_NODE_LOG, buf);
    return TRUE;
}

int do_reload_properties(struct message msg){
    //parse message to get the struct properties
    struct global_properties_struct props;
    memcpy(&props, msg.data, sizeof(struct global_properties_struct));
    
    update_properties(props);

    write_global_properties();
    return TRUE;
}

int do_notify_message(struct message msg) {
    char buf[1024];
    memset(buf, 0, 1024);
    sprintf(buf, "%s %s\n", msg.src_ip, msg.data);
    write_log(NOTIFY_LOG, buf);
    return TRUE;
}

// used for load oct tree data command from client
int do_load_data(struct message msg) {
    FILE *fp;
    char file_name[MAX_FILE_NAME];
    sprintf(file_name, "%s/data", DATA_DIR);
    fp = fopen(file_name, "rb");
	if (fp == NULL) {
		write_log(ERROR_LOG, "do_load_data: can't open data file.\n");
        return FALSE;
	}

    struct query_struct query;

    // data format in data file
    // op data_id lat lon time obj_id(traj_id)
    int i, count;
    count = 0;
	while (!feof(fp)) {
        fscanf(fp, "%d\t%d\t", &query.op, &query.data_id);
        for (i = 0; i < MAX_DIM_NUM; i++) {
            double value;
            fscanf(fp, "%lf", &value);
            query.intval[i].low = value;
            query.intval[i].high = value;
        }
        fscanf(fp, "%d", &query.trajectory_id);
        fscanf(fp, "\n");
        operate_oct_tree(query, 0);
        count++;
        if(count % 10000 == 0) {
            write_log(TORUS_NODE_LOG, "new 10000.\n");
        }
    }
	fclose(fp);

    // send message to result node;
    //
    struct message notify_msg;
    notify_msg.op = NOTIFY_MESSAGE;//RANGE_QUERY_TEST_FILTER;
    strncpy(notify_msg.src_ip, the_torus->info.ip, IP_ADDR_LENGTH);
    strncpy(notify_msg.dst_ip, result_ip, IP_ADDR_LENGTH);
    strncpy(notify_msg.stamp, "", STAMP_SIZE);

    char note[100];
    memset(note, 0, 100);
    sprintf(note, "load data success.");
    memcpy(notify_msg.data, &note, strlen(note));
    send_safe(result_sockfd, (void *) &notify_msg, sizeof(struct message), 0);

    return TRUE;
}

int do_query_torus_cluster(struct message msg) {
	int i;
	char src_ip[IP_ADDR_LENGTH], dst_ip[IP_ADDR_LENGTH];
	char stamp[STAMP_SIZE];

    // get query from message
    struct query_struct query;

    size_t cpy_len = 0;
    int hops;
	memcpy((void *)&hops, msg.data, sizeof(int));
    cpy_len += sizeof(int);

	memcpy((void *) &query, msg.data + cpy_len, sizeof(struct query_struct));
    cpy_len += sizeof(struct query_struct);

	message new_msg;

    // get the torus node ip
	get_node_ip(the_torus->info, src_ip);

	skip_list_node *sln_ptr;
	sln_ptr = the_skip_list->header;

    // all leaders has the same region
    // random choose a leader 
    int index = rand() % ACTIVE_LEADER_NUM;

	if (0 == interval_overlap(sln_ptr->leader[index].region[2], query.intval[2])) { // node searched
		if (FALSE == gen_request_stamp(stamp)) {
			return FALSE;
		}

        #ifdef WRITE_LOG
            char buf[1024];
            int len = 0;
            memset(buf, 0, 1024);
            len = sprintf(buf, "query:");
            for (i = 0; i < MAX_DIM_NUM; ++i) {
                #ifdef INT_DATA
                    len += sprintf(buf + len, "[%d, %d] ", query.intval[i].low,
                            query.intval[i].high);
                #else
                    len += sprintf(buf + len, "[%lf, %lf] ", query.intval[i].low,
                            query.intval[i].high);
                #endif
            }
            sprintf(buf + len, "\n");
            write_log(TORUS_NODE_LOG, buf);

            write_log(TORUS_NODE_LOG, "search skip list success! turn to search torus\n");
        #endif

		// turn to torus layer
        fill_message((OP)QUERY_TORUS_NODE, src_ip, msg.dst_ip, stamp, msg.data, &new_msg);
		do_query_torus_node(new_msg);

        if(query.op == RANGE_NN_QUERY || query.op == RANGE_QUERY) {

            //decide whether forward message to it's forward and backward
            if (strcmp(msg.stamp, "") == 0) {
                if ((sln_ptr->level[0].forward != NULL)
                        && (interval_overlap(
                                sln_ptr->level[0].forward->leader[index].region[2],
                                query.intval[2]) == 0)) {
                    get_node_ip(sln_ptr->level[0].forward->leader[index], dst_ip);
                    fill_message((OP)msg.op, src_ip, dst_ip, "forward", msg.data,
                            &new_msg);
                    forward_message(new_msg, 0);
                    //write_log(TORUS_NODE_LOG, "skip list forward\n");
                }

                if ((sln_ptr->level[0].backward != NULL)
                        && (interval_overlap(
                                sln_ptr->level[0].backward->leader[index].region[2],
                                query.intval[2]) == 0)) {
                    get_node_ip(sln_ptr->level[0].backward->leader[index], dst_ip);
                    fill_message((OP)msg.op, src_ip, dst_ip, "backward", msg.data,
                            &new_msg);
                    forward_message(new_msg, 0);
                    //write_log(TORUS_NODE_LOG, "skip list backward\n");
                }
            } else if (strcmp(msg.stamp, "forward") == 0) {
                if ((sln_ptr->level[0].forward != NULL)
                        && (interval_overlap(
                                sln_ptr->level[0].forward->leader[index].region[2],
                                query.intval[2]) == 0)) {
                    get_node_ip(sln_ptr->level[0].forward->leader[index], dst_ip);
                    fill_message((OP)msg.op, src_ip, dst_ip, "forward", msg.data,
                            &new_msg);
                    forward_message(new_msg, 0);
                    //write_log(TORUS_NODE_LOG, "skip list forward\n");
                }
            } else {
                if ((sln_ptr->level[0].backward != NULL)
                        && (interval_overlap(
                                sln_ptr->level[0].backward->leader[index].region[2],
                                query.intval[2]) == 0)) {
                    get_node_ip(sln_ptr->level[0].backward->leader[index], dst_ip);
                    fill_message((OP)msg.op, src_ip, dst_ip, "backward", msg.data,
                            &new_msg);
                    forward_message(new_msg, 0);
                    //write_log(TORUS_NODE_LOG, "skip list backward\n");
                }
            }
        }

	} else if (-1 == interval_overlap(sln_ptr->leader[index].region[2], query.intval[2])) { // node is on the forward of skip list
		int visit_forward = 0;
		for (i = the_skip_list->level; i >= 0; --i) {
			if ((sln_ptr->level[i].forward != NULL) && \
                    (interval_overlap(sln_ptr->level[i].forward->leader[index].region[2], query.intval[2]) <= 0)) {

				get_node_ip(sln_ptr->level[i].forward->leader[index], dst_ip);
				fill_message((OP)msg.op, src_ip, dst_ip, "forward", msg.data,
						&new_msg);

                int forward_sockfd = sln_forward_sockfd[i][index]; 
                send_safe(forward_sockfd, (void *) &new_msg, sizeof(struct message), 0);
				//forward_message(new_msg, 0);
				visit_forward = 1;
				break;
			}
		}
		if (visit_forward == 0) {
            #ifdef WRITE_LOG
                write_log(TORUS_NODE_LOG, "search failed!\n");
            #endif
		}

	} else {							// node is on the backward of skip list
		int visit_backward = 0;
		for (i = the_skip_list->level; i >= 0; --i) {
			if ((sln_ptr->level[i].backward != NULL) && \
                    (interval_overlap(sln_ptr->level[i].backward->leader[index].region[2], query.intval[2]) >= 0)) {

				get_node_ip(sln_ptr->level[i].backward->leader[index], dst_ip);
				fill_message((OP)msg.op, src_ip, dst_ip, "backward", msg.data,
						&new_msg);

                int backward_sockfd = sln_backward_sockfd[i][index]; 
                send_safe(backward_sockfd, (void *) &new_msg, sizeof(struct message), 0);
				//forward_message(new_msg, 0);
				visit_backward = 1;
				break;
			}
		}
		if (visit_backward == 0) {
            #ifdef WRITE_LOG
                write_log(TORUS_NODE_LOG, "search failed!\n");
            #endif
		}
	}

	return TRUE;
}

//return elasped time from start to finish (ns)
long get_elasped_time(struct timespec start, struct timespec end) {
	return 1000000000L * (end.tv_sec - start.tv_sec)
			+ (end.tv_nsec - start.tv_nsec);
}

int do_receive_result(struct message msg) {
	struct timespec query_end;
    double end_time = 0.0;

	char stamp[STAMP_SIZE];
	char ip[IP_ADDR_LENGTH];
	int i, count;
    size_t cpy_len = 0;

    struct query_struct query;
	memcpy(stamp, msg.stamp, STAMP_SIZE);
	memcpy(ip, msg.src_ip, IP_ADDR_LENGTH);

	memcpy(&count, msg.data, sizeof(int));
    cpy_len += sizeof(int);
	memcpy(&query, msg.data + cpy_len, sizeof(struct query_struct));
    cpy_len += sizeof(struct query_struct);
	memcpy(&query_end, msg.data + cpy_len, sizeof(struct timespec));

	char file_name[1024];
	sprintf(file_name, "/root/result/%s", stamp);
	FILE *fp = fopen(file_name, "ab+");
	if (fp == NULL) {
		printf("can't open file %s\n", file_name);
		return FALSE;
	}

	fprintf(fp, "query:");
	for (i = 0; i < MAX_DIM_NUM; i++) {
        #ifdef INT_DATA
            fprintf(fp, "[%d, %d] ", query.intval[i].low, query.intval[i].high);
        #else
            fprintf(fp, "[%lf, %lf] ", query.intval[i].low, query.intval[i].high);
        #endif
	}
	fprintf(fp, "\n");

    end_time = (double)(1000000L * query_end.tv_sec + query_end.tv_nsec/1000);
	fprintf(fp, "end query time:%f us\n", end_time);
	fprintf(fp, "%s:%d\n\n", ip, count);
	fclose(fp);

	return TRUE;
}

int do_receive_query(struct message msg) {
	struct timespec query_start;
    double start_time = 0.0;

	char stamp[STAMP_SIZE];
	char ip[IP_ADDR_LENGTH];
	int i, count;
    size_t cpy_len = 0;

    struct query_struct query;
	memcpy(stamp, msg.stamp, STAMP_SIZE);
	memcpy(ip, msg.src_ip, IP_ADDR_LENGTH);

	memcpy(&count, msg.data, sizeof(int));
    cpy_len += sizeof(int);
	memcpy(&query, msg.data + cpy_len, sizeof(struct query_struct));
    cpy_len += sizeof(struct query_struct);

	memcpy(&query_start, msg.data + cpy_len, sizeof(struct timespec));

	char file_name[1024];
	sprintf(file_name, "/root/result/%s", stamp);
	FILE *fp = fopen(file_name, "ab+");
	if (fp == NULL) {
		printf("can't open file %s\n", file_name);
		return FALSE;
	}

	fprintf(fp, "query:");
	for (i = 0; i < MAX_DIM_NUM; i++) {
        #ifdef INT_DATA
            fprintf(fp, "[%d, %d] ", query.intval[i].low, query.intval[i].high);
        #else
            fprintf(fp, "[%lf, %lf] ", query.intval[i].low, query.intval[i].high);
        #endif
	}
	fprintf(fp, "\n");

    start_time = (double)(1000000L * query_start.tv_sec + query_start.tv_nsec/1000);
	fprintf(fp, "start query time:%f us\n\n", start_time);
	fclose(fp);

	return TRUE;
}

int do_receive_filter_log(struct message msg) {
    struct filter_log_struct fl_st;
    memcpy(&fl_st, msg.data, sizeof(filter_log_struct));
    timespec cur_time;
    clock_gettime(CLOCK_REALTIME, &cur_time);

    char buf[1024];
    memset(buf, 0, 1024);
    int cpy_len = 0;
    //timestamp, query_id, region, index_elapsed, obtain_idle_elapsed, send_refinement_elapsed, 
    //requested_num, actual_got_num, size_after_index, size_after_filter
    cpy_len = sprintf(buf, "%ld,%ld,%s,%d,[", cur_time.tv_sec, cur_time.tv_nsec,msg.src_ip,fl_st.query_id);
    int i;
    for (i=0;i<MAX_DIM_NUM;i++){
        cpy_len += sprintf(buf + cpy_len, "%lf %lf ", fl_st.qry_region[i].low, fl_st.qry_region[i].high);
    }
    cpy_len += sprintf(buf + cpy_len, "]");
    cpy_len += sprintf(buf + cpy_len, ",%d,%d,%d", fl_st.index_elapsed, fl_st.obtain_idle_elapsed, fl_st.send_refinement_elapsed);
    cpy_len += sprintf(buf + cpy_len, ",%d,%d,%u,%u\n", fl_st.requested_num, fl_st.actual_got_num,\
            fl_st.size_after_index, fl_st.size_after_filter); 

    write_log(filter_log, buf);
    return TRUE;
}

int do_receive_refinement_log(struct message msg) {
    struct refinement_log_struct rfmt_st;
    memcpy(&rfmt_st, msg.data, sizeof(refinement_log_struct));
    timespec cur_time;
    clock_gettime(CLOCK_REALTIME, &cur_time);

    char buf[1024];
    memset(buf, 0, 1024);
    int cpy_len = 0;
    //timestamp, query_id, region, rece_data_elapsed, calc_qp_elapsed, result_size, avg_qp 
    cpy_len = sprintf(buf, "%ld,%ld,%s,%d,[", cur_time.tv_sec, cur_time.tv_nsec,msg.src_ip,rfmt_st.query_id);
    int i;
    for (i=0;i<MAX_DIM_NUM;i++){
        cpy_len += sprintf(buf + cpy_len, "%lf %lf ", rfmt_st.qry_region[i].low, rfmt_st.qry_region[i].high);
    }
    cpy_len += sprintf(buf + cpy_len, "],");
    cpy_len += sprintf(buf + cpy_len, "%d,%d,%u,%lf\n", rfmt_st.recv_refinement_elapsed, rfmt_st.calc_qp_elapsed, \
            rfmt_st.result_size, rfmt_st.avg_qp);

    write_log(refinement_log, buf);
    return TRUE;
}

int do_throughput_test(struct message msg) {
    static int count = 0;

    char buffer[100];
    memset(buffer, 0, 100);
    int cur_count;
    double cur_hops;
    double cur_el;
    coordinate cor;
    int cpy_len = 0;
    memcpy(&cur_count, msg.data, sizeof(int));
    cpy_len += sizeof(int);

    memcpy(&cur_hops, msg.data + cpy_len, sizeof(double));
    cpy_len += sizeof(double);

    memcpy(&cur_el, msg.data + cpy_len, sizeof(double));
    cpy_len += sizeof(double);

    memcpy(&cor, msg.data + cpy_len, sizeof(coordinate));
    count += cur_count;

    sprintf(buffer, "[%d%d%d] %d %lf %lf %d\n", cor.x, cor.y, cor.z, (int)time(NULL), cur_el, cur_hops, count);
    write_log(result_log, buffer);
    return TRUE;
}

int do_trajectory_query(connection_t conn, struct message msg) {
    struct query_struct query;
    int has_point = 0;

    char *ptr = msg.data; 
    memcpy(&query, ptr, sizeof(struct query_struct));
    ptr += sizeof(struct query_struct);

    OctPoint pt;
    pt.loadFromByteArray(ptr);

    OctPoint *point = NULL;
    hash_map<IDTYPE, Traj*>::iterator it;
    it = g_TrajList.find(query.trajectory_id);
    if(it != g_TrajList.end()) {
        has_point = 1;
        Traj *traj = it->second;
        // TODO tail or head
        point = g_PtList.find(traj->t_tail)->second;

        OctPoint *new_point = new OctPoint();
        the_torus_oct_tree->geneBorderPoint(point, &pt, new_point, the_torus_oct_tree->tree_domLow, the_torus_oct_tree->tree_domHigh);

        point->next = new_point->p_id;
        new_point->pre = point->p_id;
        new_point->next = 0;

        g_ptNewCount++;
        new_point->p_id = -g_ptNewCount;

    } 

    int cpy_len = 0;
    byte buf[SOCKET_BUF_SIZE];
    memset(buf, 0, SOCKET_BUF_SIZE);
    memcpy(buf, &has_point, sizeof(int));
    cpy_len += sizeof(int);

    if(1 == has_point) {
        char *package_ptr;
        uint32_t package_len;
        point->storeToByteArray(&package_ptr, package_len);
        memcpy(buf + cpy_len, package_ptr, package_len);
        cpy_len += package_len;

        delete[] package_ptr;
    }
    send_safe(conn->socketfd, (void *)buf, SOCKET_BUF_SIZE, 0);
    return TRUE;
}
int do_heartbeat(struct message msg) {
    struct node_stat stat;
    struct torus_stat *ptr;

    memcpy(&stat, msg.data, sizeof(struct node_stat));

    // check if coming heartbeat record in the_torus_stat
    // if true update the stat information, or insert a new stat
    if(the_torus_stat == NULL) {
        write_log(ERROR_LOG, "do_heartbeat: the_torus_stat is null.\n");
        return FALSE;
    }
    ptr = find_node_stat(the_torus_stat, stat);
    if(ptr == NULL) {
        insert_node_stat(the_torus_stat, stat);
    } else {
        ptr->node.fvalue = stat.fvalue;
    }

    char buf[200];
    memset(buf, 0, 200);
    sprintf(buf, "%s %d\n", stat.ip, stat.fvalue);
    write_log(HEARTBEAT_LOG, buf);
    return TRUE;
}

int do_seek_idle_node(connection_t conn, struct message msg) {
    int route_count;
    // get route_count from msg and decrease route_count
    memcpy(&route_count, msg.data, sizeof(int));
    route_count--;
    int requested_num = 0;
    memcpy(&requested_num, msg.data + sizeof(int), sizeof(int));    

	struct message reply_msg;

    char idle_ip[requested_num][IP_ADDR_LENGTH];
    memset(idle_ip, 0, requested_num * IP_ADDR_LENGTH);

    int i;
    int actual_got_num = 0;
    int ret = get_idle_torus_node(idle_ip,requested_num,&actual_got_num);
    if(ret == FALSE) {
        actual_got_num = 0;
        /*if(route_count == 0) {
            reply_msg.reply_code = FAILED;
        } else {
            // route to it's backward leader
            skip_list_node *backward;
            backward = the_skip_list->header->level[0].backward;
            if(backward != NULL) {

                //rand choose a leader
                i = rand() % ACTIVE_LEADER_NUM;
                get_node_ip(backward->leader[i], dst_ip);

                int backward_sockfd = sln_backward_sockfd[0][i]; 
                //socketfd = new_client_socket(dst_ip, MANUAL_WORKER_PORT);
                if (TRUE == backward_sockfd) {
                    strncpy(msg.src_ip, the_torus->info.ip, IP_ADDR_LENGTH);
                    strncpy(msg.dst_ip, dst_ip, IP_ADDR_LENGTH);
                    memcpy(msg.data, &route_count, sizeof(int));

                    send_message(backward_sockfd, msg);
                    struct reply_message reply;
                    if(TRUE == receive_reply(backward_sockfd, &reply)) {
                        if (SUCCESS != reply.reply_code) {
                            reply_msg.reply_code = FAILED;
                        } else {
                            memcpy(idle_ip, reply.stamp, sizeof(IP_ADDR_LENGTH));
                        }
                    }
                    close(backward_sockfd);
                }
            } else {
                reply_msg.reply_code = FAILED;
            }
        }*/
    } 

    reply_msg.op = (OP)msg.op;
    strncpy(reply_msg.src_ip, msg.dst_ip, IP_ADDR_LENGTH); 
    strncpy(reply_msg.dst_ip, msg.src_ip, IP_ADDR_LENGTH);
    strncpy(reply_msg.stamp, "", STAMP_SIZE);
    int cpy_len = 0;
    memcpy(reply_msg.data + cpy_len, &actual_got_num, sizeof(int)); 
    cpy_len += sizeof(int);
    for (i = 0; i < actual_got_num; i++){
        memcpy(reply_msg.data + cpy_len, idle_ip[i], IP_ADDR_LENGTH); 
        cpy_len += IP_ADDR_LENGTH;    
    }
    send_message(conn->socketfd, reply_msg);
    return TRUE;
}

int process_message(connection_t conn, struct message msg) {

	struct reply_message reply_msg;
	REPLY_CODE reply_code = SUCCESS;
	switch (msg.op) {

	case CREATE_TORUS:
        #ifdef WRITE_LOG
            write_log(TORUS_NODE_LOG, "receive request update torus.\n");
        #endif
		if (FALSE == do_create_torus(msg)) {
			reply_code = FAILED;
		}
		break;

	case UPDATE_PARTITION:
        #ifdef WRITE_LOG
            write_log(TORUS_NODE_LOG, "receive request update torus.\n");
        #endif
		if (FALSE == do_update_partition(msg)) {
			reply_code = FAILED;
		}

		reply_msg.op = (OP)msg.op;
		reply_msg.reply_code = (REPLY_CODE)reply_code;
		strncpy(reply_msg.stamp, msg.stamp, STAMP_SIZE);

		if (FALSE == send_reply(conn->socketfd, reply_msg)) {
			// TODO handle send reply failed
			return FALSE;
		}
		break;

	case TRAVERSE_TORUS:
        #ifdef WRITE_LOG
            write_log(TORUS_NODE_LOG, "receive request traverse torus.\n");
        #endif

		do_traverse_torus(msg);
		break;

	case QUERY_TORUS_NODE:
        #ifdef WRITE_LOG
            write_log(TORUS_NODE_LOG, "\nreceive request search torus node.\n");
        #endif

		do_query_torus_node(msg);
		break;

	case UPDATE_SKIP_LIST:
        #ifdef WRITE_LOG
            write_log(TORUS_NODE_LOG, "receive request update skip list.\n");
        #endif

		do_update_skip_list(msg);
		//print_skip_list_node(the_skip_list);
		break;

	case UPDATE_SKIP_LIST_NODE:
        #ifdef WRITE_LOG
            write_log(TORUS_NODE_LOG, "receive request update skip list node.\n");
        #endif

		do_update_skip_list_node(msg);
		print_skip_list_node(the_skip_list);
		break;

	case QUERY_TORUS_CLUSTER:
        #ifdef WRITE_LOG
            write_log(TORUS_NODE_LOG, "\nreceive request search skip list node.\n");
        #endif
        /*static int count = 0;
        count++;
        if(count % 10000 == 0) {
            char b[10];
            sprintf(b, "%d\n", count);
            write_log(TORUS_NODE_LOG, b);
        }*/

		do_query_torus_cluster(msg);
		break;

    case LOAD_DATA:
        #ifdef WRITE_LOG
            write_log(TORUS_NODE_LOG, "\nreceive request search skip list node.\n");
        #endif
		do_load_data(msg);
        break;

	case NEW_SKIP_LIST:
        #ifdef WRITE_LOG
            write_log(TORUS_NODE_LOG, "receive request new skip list.\n");
        #endif
		do_new_skip_list(msg);
		print_skip_list_node(the_skip_list);
		break;

	case TRAVERSE_SKIP_LIST:
        #ifdef WRITE_LOG
            write_log(TORUS_NODE_LOG, "receive request traverse skip list.\n");
        #endif

		do_traverse_skip_list(msg);
		break;

    case THROUGHPUT_TEST:
        #ifdef WRITE_LOG
            write_log(TORUS_NODE_LOG, "receive request throughput test.\n");
        #endif

        do_throughput_test(msg);
        break;

    case RECEIVE_FILTER_LOG:
        #ifdef WRITE_LOG
            write_log(TORUS_NODE_LOG, "receive request range query test filter.\n");
        #endif

        do_receive_filter_log(msg);
        break;

    case RECEIVE_REFINEMENT_LOG:
        #ifdef WRITE_LOG
            write_log(TORUS_NODE_LOG, "receive request range query test refinement.\n");
        #endif

        do_receive_refinement_log(msg);
        break;
    
    case HEARTBEAT:
        #ifdef WRITE_LOG
            write_log(TORUS_NODE_LOG, "receive heartbeat information.\n");
        #endif

        do_heartbeat(msg);
        break;

    case SEEK_IDLE_NODE:
        #ifdef WRITE_LOG
            write_log(TORUS_NODE_LOG, "receive heartbeat information.\n");
        #endif

        do_seek_idle_node(conn, msg);
        break;

    case RELOAD_RTREE:
        #ifdef WRITE_LOG
            write_log(TORUS_NODE_LOG, "receive request reload rtree data.\n");
        #endif
        do_rtree_load_data(conn, msg);
        break;

    case LOAD_OCT_TREE_POINTS:
        #ifdef WRITE_LOG
            write_log(TORUS_NODE_LOG, "receive request reload oct tree points.\n");
        #endif
        do_load_oct_tree_points(conn, msg);
        break;

    case LOAD_OCT_TREE_NODES:
        #ifdef WRITE_LOG
            write_log(TORUS_NODE_LOG, "receive request reload oct tree nodes.\n");
        #endif
        do_load_oct_tree_nodes(conn, msg);
        break;

    case LOAD_OCT_TREE_TRAJECTORYS:
        #ifdef WRITE_LOG
            write_log(TORUS_NODE_LOG, "receive request reload oct tree trajectorys.\n");
        #endif
        do_load_oct_tree_trajectorys(conn, msg);
        break;

    case TRAJ_QUERY:
        //conn->used = 0;
        #ifdef WRITE_LOG
            write_log(TORUS_NODE_LOG, "receive request query oct tree trajectory.\n");
        #endif
        do_trajectory_query(conn, msg);
        //conn->used = 1;
        break;

    case RANGE_QUERY_REFINEMENT:
        #ifdef WRITE_LOG
            write_log(TORUS_NODE_LOG, "receive request calc range query refinement.\n");
        #endif
        do_range_query_refinement(conn, msg);
        break;

    case NN_QUERY_REFINEMENT:
        #ifdef WRITE_LOG
            write_log(TORUS_NODE_LOG, "receive request calc nn query refinement.\n");
        #endif
        do_nn_query_refinement(conn, msg);
        break;

    case NOTIFY_MESSAGE:
        #ifdef WRITE_LOG
            write_log(TORUS_NODE_LOG, "receive request notify message .\n");
        #endif
        do_notify_message(msg);
        break;
    
    case RELOAD_PROPERTIES:
        #ifdef WRITE_LOG
            write_log(TORUS_NODE_LOG, "receive request reload properties message .\n");
        #endif
        do_reload_properties(msg);
        break;

	default:
		reply_code = (REPLY_CODE)WRONG_OP;
        break;
	}


	return TRUE;
}

int new_server(int port) {
	int server_socket;
	server_socket = new_server_socket(port);
	if (server_socket == FALSE) {
		return FALSE;
	}
	return server_socket;
}

int handle_read_event(connection_t conn) {
    if(conn->roff == CONN_BUF_SIZE) {
        return FALSE;
    }
    int ret = recv(conn->socketfd, conn->rbuf + conn->roff, CONN_BUF_SIZE - conn->roff, 0);

    if(ret > 0) {
        conn->roff += ret;

        int beg = 0;
        int msg_size = sizeof(struct message);
        struct message msg;
        while(beg + msg_size <= conn->roff) {
            memcpy(&msg, conn->rbuf + beg, msg_size);
            beg = beg + msg_size;

            //process message
            process_message(conn, msg);
        }
        int left = conn->roff - beg;
        if( beg != 0 && left > 0) {
            memmove(conn->rbuf, conn->rbuf + beg, left);
        }
        conn->roff = left;
    } else if(ret == 0){
        return FALSE;
    } else {
        if(errno != EINTR && errno != EAGAIN) {
            return FALSE;
        }
    }
    return TRUE;
}

void *manual_worker_monitor(void *args) {
	// add manual_worker_listener 
	int epfd;
	struct epoll_event ev, events[MAX_EVENTS];
	epfd = epoll_create(MAX_EVENTS);
	ev.data.fd = manual_worker_socket;
	ev.events = EPOLLIN;
	epoll_ctl(epfd, EPOLL_CTL_ADD, manual_worker_socket, &ev);

	int nfds;
    int conn_socket;

    // manual worker index from 0 ~ MANUAL_WORKER - 1
    int index = 0;
    int i;

	while(should_run) {
		nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);
		for(i = 0; i < nfds; i++) {
        //if(nfds > 0) {
            while((conn_socket = accept_connection(manual_worker_socket)) > 0) {
                set_nonblocking(conn_socket);
                ev.events = EPOLLIN;// | EPOLLONESHOT;
                ev.data.fd = conn_socket;

                /* register conn_socket to worker-pool's 
                 * epoll, not the listen epoll*/
                g_conn_table[conn_socket].used = 1;
                
                // record enter_time of coming connection
                clock_gettime(CLOCK_REALTIME, &g_conn_table[conn_socket].enter_time);

                g_conn_table[conn_socket].index = index; 
                g_conn_table[conn_socket].socketfd = conn_socket; 
                epoll_ctl(worker_epfd[index], EPOLL_CTL_ADD, conn_socket, &ev);

                index = (index + 1) % MANUAL_WORKER;
            }
		} 
	}

    close(epfd);
    return NULL;
}

void close_connection(connection_t conn) {
    //if(conn->used != 0) {
    struct epoll_event ev;
    conn->used = 0;
    conn->roff = 0;
    epoll_ctl(worker_epfd[conn->index], EPOLL_CTL_DEL, conn->socketfd, &ev);
    close(conn->socketfd);
}

void update_max_fvalue(struct refinement_stat r_stat, struct query_struct query, int op) {
    // estimate response time based on the r_stat and query type
    int res_time = estimate_response_time(r_stat, query);
    int val = res_time * op;

    pthread_mutex_lock(&global_variable_mutex);
    the_node_stat.fvalue += val;
    pthread_mutex_unlock(&global_variable_mutex);
}

void *worker(void *args){
    int epfd = *(int *)args;

	struct epoll_event events[MAX_EVENTS], ev;

    int i, nfds;
    struct message msg;
    int conn_socket;

	while(should_run) {
		nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);
		for(i = 0; i < nfds; i++) {
        //if(nfds > 0) {
            if (events[i].events && EPOLLIN) {
				conn_socket = events[i].data.fd;

                connection_t conn = &g_conn_table[conn_socket];

                // handle manual task (connection closed here)
                if(conn->index < MANUAL_WORKER) {
                    if(FALSE == handle_read_event(conn)) {
                        close_connection(conn);
                        continue;
                    }

                    if(conn->used != 0) {
                        ev.events = EPOLLIN;// | EPOLLONESHOT;
                        ev.data.fd = conn_socket;
                        epoll_ctl(epfd, EPOLL_CTL_MOD, conn->socketfd, &ev);
                    }
                } else {
                    // handle compute task (connection closed by receiver)
                    memset(&msg, 0, sizeof(struct message));

                    // receive message through the conn_socket
                    if (TRUE == receive_message(conn->socketfd, &msg)) {
                        process_message(conn, msg);
                    } else {
                        write_log(ERROR_LOG, "receive compute task request failed.\n");
                        close_connection(conn);
                    }

                }
			} else if(events[i].events && EPOLLRDHUP) {
				conn_socket = events[i].data.fd;
                connection_t conn = &g_conn_table[conn_socket];
                close_connection(conn);
            }

		}
	}

    return NULL;
}

void *compute_worker_monitor(void *args) {
	// listen epoll
	int epfd;
	struct epoll_event ev, events[MAX_EVENTS];
	epfd = epoll_create(MAX_EVENTS);
	ev.data.fd = compute_worker_socket;
	ev.events = EPOLLIN | EPOLLET;
	epoll_ctl(epfd, EPOLL_CTL_ADD, compute_worker_socket, &ev);

	int nfds;
    int conn_socket;

    // epoll index from MANUAL_WORKER ~ (WORKER_NUM - 1) used for compute worker
    int index = MANUAL_WORKER;
    int i;

	while(should_run) {
		nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);
		for(i = 0; i < nfds; i++) {
        //if(nfds > 0) {
            if(events[i].data.fd == compute_worker_socket) {
                while((conn_socket = accept_connection(compute_worker_socket)) > 0) {
                    set_nonblocking(conn_socket);
                    ev.events = EPOLLIN | EPOLLET;// | EPOLLONESHOT;
                    ev.data.fd = conn_socket;

                    g_conn_table[conn_socket].used = 1;
                    // record enter_time of coming connection
                    //clock_gettime(CLOCK_REALTIME, &g_conn_table[conn_socket].enter_time);

                    g_conn_table[conn_socket].index = index; 
                    g_conn_table[conn_socket].socketfd = conn_socket; 
                    epoll_ctl(worker_epfd[index], EPOLL_CTL_ADD, conn_socket, &ev);

                    index = (index + 1) % COMPUTE_WORKER;
                    if(index == 0) {
                        index = MANUAL_WORKER;
                    }
                }
            }
		} 
	}

    close(epfd);
    return NULL;
}

int send_node_status(const char *dst_ip, struct node_stat stat) {
    int socketfd;
    socketfd = find_leader_sock(dst_ip);
	//socketfd = new_client_socket(dst_ip, MANUAL_WORKER_PORT);
	if (FALSE == socketfd) {
        write_log(ERROR_LOG, "send_node_status: find leader sock failed");
		return FALSE;
	}

	// get local ip address
	char local_ip[IP_ADDR_LENGTH];
	memset(local_ip, 0, IP_ADDR_LENGTH);
	if (FALSE == get_local_ip(local_ip)) {
		return FALSE;
	}

    struct message msg;
    msg.op = HEARTBEAT;
    strncpy(msg.src_ip, local_ip, IP_ADDR_LENGTH);
    strncpy(msg.dst_ip, dst_ip, IP_ADDR_LENGTH);
    strncpy(msg.stamp, "", STAMP_SIZE);


    memcpy(msg.data, &stat, sizeof(struct node_stat));

    send_message(socketfd, msg);
    return TRUE;
}

void send_heartbeat() {
    int i;
    for(i = 0; i < ACTIVE_LEADER_NUM; i++) {
        if(strcmp(the_torus_leaders[i].ip, "") == 0){
            break;
        }
        pthread_mutex_lock(&global_variable_mutex);
        if(strcmp(the_node_stat.ip, "") == 0) {
            break;
        }
        struct node_stat stat = the_node_stat;
        //the_node_stat.fvalue = 0;
        pthread_mutex_unlock(&global_variable_mutex);

        if(is_active_leader() == TRUE) {
            stat.fvalue = INT_MAX;
        }
        send_node_status(the_torus_leaders[i].ip, stat);
    }
}

void *heartbeat_worker(void *args){
    while(should_run) {
        //send heart beat
        send_heartbeat();
        sleep(HEARTBEAT_INTERVAL);
    }
    return NULL;
}

int main(int argc, char **argv) {
	write_log(TORUS_NODE_LOG, "start torus node.\n");

    // step 1: read global properties file 
    struct global_properties_struct props;
    int ret = read_properties(&props);
    if(ret == FALSE) {
        exit(0);
    }
    update_properties(props);
	write_log(TORUS_NODE_LOG, "read global properties.\n");
    write_global_properties();

    // only for test
    struct tm *local;
    time_t t = time(NULL);
    local = localtime(&t);
    sprintf(result_log, "../logs/result_%d%d%d%d%d.log", local->tm_mon + 1, local->tm_mday, local->tm_hour, \
            local->tm_min, local->tm_sec);

    sprintf(filter_log, "../logs/filter_%d%d%d%d%d.log", local->tm_mon + 1, local->tm_mday, local->tm_hour, \
            local->tm_min, local->tm_sec);

    sprintf(refinement_log, "../logs/refinement_%d%d%d%d%d.log", local->tm_mon + 1, local->tm_mday, \
            local->tm_hour, local->tm_min, local->tm_sec);


    // step 2: initialize global variables
	int i, j;
	should_run = 1;

	// init the torus ptr 
	the_torus = NULL; 

    // init the torus's leaders info
    for(i = 0; i < LEADER_NUM; i++) {
        memset(the_torus_leaders[i].ip, 0, IP_ADDR_LENGTH);
    }

	// init the skip list ptr 
	the_skip_list = NULL; 

    // init the skip list node socket fd 
    for(i = 0; i < MAXLEVEL; i++) {
        for(j = 0; j < LEADER_NUM; j++) {
            sln_forward_sockfd[i][j] = -1;
            sln_backward_sockfd[i][j] = -1;
        }
    }

	// create a new request list
	req_list = new_request();

	// create rtree
    /*the_torus_rtree = NULL;
	the_torus_rtree = rtree_create();
    if(the_torus_rtree == NULL){
        write_log(TORUS_NODE_LOG, "create rtree failed.\n");
        exit(1);
    }
	write_log(TORUS_NODE_LOG, "create rtree.\n");*/

    // init oct_tree
    // do nothing, all structure are stored in global hash_maps in OctTree;
	the_torus_oct_tree = NULL;

    // init torus neighbors socket
    neighbor_sock_num = 0;
    for(i = 0; i < MAX_NEIGHBORS; i++) {
        memset(neighbor_sock[i].ip, 0, IP_ADDR_LENGTH);
        neighbor_sock[i].sockfd = -1;
    }

    // init mutex;
    pthread_mutex_init(&mutex, NULL);
    pthread_mutex_init(&global_variable_mutex, NULL);

    // create two server socket to process different kind of requests
	manual_worker_socket = new_server(MANUAL_WORKER_PORT);
	if (manual_worker_socket == FALSE) {
        write_log(TORUS_NODE_LOG, "start manual-worker server failed.\n");
        exit(1);
	}
	printf("start manual-worker server.\n");
	write_log(TORUS_NODE_LOG, "start manual-worker server.\n");

	compute_worker_socket = new_server(COMPUTE_WORKER_PORT);
	if (compute_worker_socket == FALSE) {
        write_log(TORUS_NODE_LOG, "start compute-worker server failed.\n");
        exit(1);
	}
	printf("start compute-worker server.\n");
	write_log(TORUS_NODE_LOG, "start compute-worker server.\n");

	// set server socket nonblocking
	set_nonblocking(manual_worker_socket);
	set_nonblocking(compute_worker_socket);


    // step 3: create several threads and start service for client 
    // create listen thread for epoll listerner
    pthread_create(&manual_worker_monitor_thread, NULL, manual_worker_monitor, NULL);

    //create EPOLL_NUM different epoll fd for workers
    for (i = 0; i < EPOLL_NUM; i++) {
        worker_epfd[i] = epoll_create(MAX_EVENTS);
    }

    // create worker thread to handle ready socket fds
    for(i = 0; i < EPOLL_NUM; i++) {
        int j; 
        for(j = 0; j < WORKER_PER_GROUP; j++) {
            pthread_create(worker_thread + (i * WORKER_PER_GROUP + j), NULL, worker, worker_epfd + i);
        }
    }

    // create data thread to handle data request
    pthread_create(&compute_worker_monitor_thread, NULL, compute_worker_monitor, NULL);

    // create heartbeat thread to send status info
    pthread_create(&heartbeat_thread, NULL, heartbeat_worker, NULL);

    pthread_join(heartbeat_thread, NULL);
    pthread_join(compute_worker_monitor_thread, NULL);
    for (i = 0; i < WORKER_NUM; i++) {
        pthread_join(worker_thread[i], NULL);
    }
    pthread_join(manual_worker_monitor_thread, NULL);

    // close worker epoll fds
    for (i = 0; i < EPOLL_NUM; i++) {
        close(worker_epfd[i]);
    }

    close(manual_worker_socket);

	return 0;
}

