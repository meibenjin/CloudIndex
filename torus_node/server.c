/*
 * server.c
 *
 *  Created on: Sep 24, 2013
 *      Author: meibenjin
 */

__asm__(".symver memcpy,memcpy@GLIBC_2.2.5");

extern "C" {
#include"server.h"
#include"logs/log.h"
#include"torus_node/torus_node.h"
#include"skip_list/skip_list.h"
#include"socket/socket.h"
}; 

#include"torus_rtree.h"
#include"oct_tree/OctTree.h"

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/time.h>
#include<time.h>
#include<unistd.h>
#include<sys/epoll.h>
#include<pthread.h>
#include<errno.h>


//define torus server 
/*****************************************************************************/

torus_node *the_torus;
struct torus_partitions the_partition;

ISpatialIndex* the_torus_rtree;

OctTree *the_torus_oct_tree;

// mark torus node is active or not 
int should_run;

struct skip_list *the_skip_list;


// handle request variablies
struct connection_st g_conn_table[CONN_MAXFD];

pthread_mutex_t mutex;
//pthread_cond_t condition;

// ready epoll fds
int worker_epfd[EPOLL_NUM];
int server_socket;
int data_socket;

pthread_t listener_thread;
pthread_t worker_thread[WORKER_NUM];


char result_ip[IP_ADDR_LENGTH] = "172.16.0.83";

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
	// TODO automatic generate number stamp
	static long number_stamp = 1;
	char ip_stamp[IP_ADDR_LENGTH];
	if (FALSE == get_local_ip(ip_stamp)) {
		return FALSE;
	}
	snprintf(stamp, STAMP_SIZE, "%s_%ld", ip_stamp, number_stamp++);
	return TRUE;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

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
	int i, d;
    size_t cpy_len = 0;
	int num, neighbors_num = 0;

    // allocate for the_torus if it's NULL
    if(the_torus == NULL) {
        the_torus = new_torus_node();
    }

    // get torus partitons info from msg
    memcpy(&the_partition, (void *)msg.data, sizeof(struct torus_partitions));
    cpy_len += sizeof(struct torus_partitions);

    #ifdef WRITE_LOG 
        char buf[1024];
        sprintf(buf, "torus partitions:[%d %d %d]\n", the_partition.p_x,
                the_partition.p_y, the_partition.p_z);
        write_log(TORUS_NODE_LOG, buf);
    #endif

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
        }
        neighbors_num += num;
    }

    // set the_torus's neighbors_num
	set_neighbors_num(the_torus, neighbors_num);

    #ifdef WRITE_LOG 
        sprintf(buf, "torus node capacity:%d\n", the_torus->info.capacity);
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
	int i, index, nodes_num, level;

	nodes_num = 0;
	memcpy(&nodes_num, msg.data, sizeof(int));
	if (nodes_num <= 0) {
		printf("do_update_skip_list: skip_list node number is wrong\n");
		return FALSE;
	}

	skip_list_node *sln_ptr, *new_sln;
	node_info nodes[nodes_num];

	for (i = 0; i < nodes_num; ++i) {
		memcpy(&nodes[i],
				(void*) (msg.data + sizeof(int) + sizeof(node_info) * i),
				sizeof(node_info));
	}

	level = (nodes_num / 2) - 1;
	index = 0;
	sln_ptr = the_skip_list->header;

	for (i = level; i >= 0; i--) {
		if (get_cluster_id(nodes[index]) != -1) {
			new_sln = new_skip_list_node(0, &nodes[index]);
			// free old forward field first
			if (sln_ptr->level[i].forward) {
				free(sln_ptr->level[i].forward);
			}
			sln_ptr->level[i].forward = new_sln;
			index++;
		} else {
			index++;
		}
		if (get_cluster_id(nodes[index]) != -1) {
			new_sln = new_skip_list_node(0, &nodes[index]);
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
	char src_ip[IP_ADDR_LENGTH], dst_ip[IP_ADDR_LENGTH];
	skip_list_node *cur_sln, *forward, *backward;
	cur_sln = the_skip_list->header;
	forward = cur_sln->level[0].forward;
	backward = cur_sln->level[0].backward;

    #ifdef WRITE_LOG
        write_log(TORUS_NODE_LOG, "visit myself:");
    #endif
	print_node_info(cur_sln->leader);

	get_node_ip(cur_sln->leader, src_ip);
	strncpy(msg.src_ip, src_ip, IP_ADDR_LENGTH);
	if (strcmp(msg.data, "") == 0) {
		if (forward) {
			get_node_ip(forward->leader, dst_ip);
			strncpy(msg.dst_ip, dst_ip, IP_ADDR_LENGTH);
			strncpy(msg.data, "f", DATA_SIZE);
			forward_message(msg, 0);
		}

		if (backward) {
			get_node_ip(backward->leader, dst_ip);
			strncpy(msg.dst_ip, dst_ip, IP_ADDR_LENGTH);
			strncpy(msg.data, "b", DATA_SIZE);
			forward_message(msg, 0);
		}

	} else if (strcmp(msg.data, "f") == 0) {
		if (forward) {
			get_node_ip(forward->leader, dst_ip);
			strncpy(msg.dst_ip, dst_ip, IP_ADDR_LENGTH);
			forward_message(msg, 0);
		}

	} else {
		if (backward) {
			get_node_ip(backward->leader, dst_ip);
			strncpy(msg.dst_ip, dst_ip, IP_ADDR_LENGTH);
			forward_message(msg, 0);
		}
	}

	return TRUE;
}

int do_update_skip_list_node(struct message msg) {
	int i;
    size_t cpy_len = 0;
	// analysis node info from message
	memcpy(&i, msg.data, sizeof(int));
    cpy_len += sizeof(int);

	node_info f_node, b_node;
	memcpy(&f_node, (void*)(msg.data + cpy_len), sizeof(node_info));
    cpy_len += sizeof(node_info);

	memcpy(&b_node, (void*)(msg.data + cpy_len), sizeof(node_info));
    cpy_len += sizeof(node_info);

	skip_list_node *sln_ptr, *new_sln;
	sln_ptr = the_skip_list->header;
	if (get_cluster_id(f_node) != -1) {
		new_sln = new_skip_list_node(0, &f_node);

		// free old forward field first
		if (sln_ptr->level[i].forward) {
			free(sln_ptr->level[i].forward);
		}

		sln_ptr->level[i].forward = new_sln;
	}

	if (get_cluster_id(b_node) != -1) {
		new_sln = new_skip_list_node(0, &b_node);

		// free old backward field first
		if (sln_ptr->level[i].backward) {
			free(sln_ptr->level[i].backward);
		}
		sln_ptr->level[i].backward = new_sln;
	}
	return TRUE;
}
int do_update_forward(struct message msg) {
	int i;
    size_t cpy_len = 0;

	// get node info from message
	memcpy(&i, msg.data, sizeof(int));
    cpy_len += sizeof(int);

	node_info f_node;
	memcpy(&f_node, (void*) (msg.data + cpy_len), sizeof(node_info));
    cpy_len += sizeof(node_info);

	skip_list_node *sln_ptr, *new_sln;
	sln_ptr = the_skip_list->header;
	if (get_cluster_id(f_node) != -1) {
		new_sln = new_skip_list_node(0, &f_node);

		// free old forward field first
		if (sln_ptr->level[i].forward) {
			free(sln_ptr->level[i].forward);
		}

		sln_ptr->level[i].forward = new_sln;
	}
	return TRUE;
}

int do_update_backward(struct message msg) {
	int i;
    size_t cpy_len = 0;

	// get node info from message
	memcpy(&i, msg.data, sizeof(int));
    cpy_len += sizeof(int);

	node_info b_node;
	memcpy(&b_node, (void*) (msg.data + cpy_len), sizeof(node_info));
    cpy_len += sizeof(node_info);

	skip_list_node *sln_ptr, *new_sln;
	sln_ptr = the_skip_list->header;
	if (get_cluster_id(b_node) != -1) {
		new_sln = new_skip_list_node(0, &b_node);

		// free old backward field first
		if (sln_ptr->level[i].backward) {
			free(sln_ptr->level[i].backward);
		}

		sln_ptr->level[i].backward = new_sln;
	}
	return TRUE;
}

int do_new_skip_list(struct message msg) {
	int level;
    size_t cpy_len = 0;

	// get node info from message
	skip_list_node *sln_ptr;
	node_info leader;

	memcpy(&level, msg.data, sizeof(int));
    cpy_len += sizeof(int);
	memcpy(&leader, msg.data + cpy_len, sizeof(node_info));
    cpy_len += sizeof(node_info);

    if(the_skip_list == NULL) {
        the_skip_list = new_skip_list(level);
    }
    sln_ptr = the_skip_list->header;
    sln_ptr->leader = leader;
    sln_ptr->height = level;
    the_skip_list->level = level;
	return TRUE;
}

int forward_search(struct interval intval[], struct message msg, int d) {
	char src_ip[IP_ADDR_LENGTH], dst_ip[IP_ADDR_LENGTH];

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

    #ifdef WRITE_LOG 
        char buf[1024];
        int len = 0, i;
        len = sprintf(buf, "query:");
        for (i = 0; i < MAX_DIM_NUM; ++i) {
            #ifdef INT_DATA
                len += sprintf(buf + len, "[%d, %d] ", intval[i].low,
                        intval[i].high);
            #else
                len += sprintf(buf + len, "[%lf, %lf] ", intval[i].low,
                        intval[i].high);
            #endif
        }
        sprintf(buf + len, "\n");
    #endif

	int lower_overlap = interval_overlap(intval[d], lower_neighbor->region[d]);
	int upper_overlap = interval_overlap(intval[d], upper_neighbor->region[d]);

	// choose forward neighbor if query overlap one neighbor at most
	if ((lower_overlap != 0) || (upper_overlap != 0)) {

		//current torus node is the first node on dimension d
		if ((the_torus->info.region[d].low < lower_neighbor->region[d].low)
				&& (the_torus->info.region[d].low
						< upper_neighbor->region[d].low)) {
			if (get_distance(intval[d], lower_neighbor->region[d])
					< get_distance(intval[d], upper_neighbor->region[d])) {
				upper_neighbor = lower_neighbor;
			}
			lower_neighbor = NULL;

		} else if ((the_torus->info.region[d].high
				> lower_neighbor->region[d].high)
				&& (the_torus->info.region[d].high
						> upper_neighbor->region[d].high)) {
			if (get_distance(intval[d], upper_neighbor->region[d])
					< get_distance(intval[d], lower_neighbor->region[d])) {
				lower_neighbor = upper_neighbor;
			}
			upper_neighbor = NULL;
		} else {
			if (lower_overlap != 0) {
				lower_neighbor = NULL;
			}
			if (upper_overlap != 0) {
				upper_neighbor = NULL;
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

    if(lower_neighbor ==  upper_neighbor) {
        lower_neighbor = NULL;
    }

	if (lower_neighbor != NULL) {
		struct message new_msg;
		get_node_ip(the_torus->info, src_ip);
		get_node_ip(*lower_neighbor, dst_ip);
		fill_message((OP)msg.op, src_ip, dst_ip, msg.stamp, msg.data, &new_msg);

        #ifdef WRITE_LOG
            write_log(TORUS_NODE_LOG, buf);
        #endif
		forward_message(new_msg, 0);
	}
	if (upper_neighbor != NULL) {
		struct message new_msg;
		get_node_ip(the_torus->info, src_ip);
		get_node_ip(*upper_neighbor, dst_ip);
		fill_message((OP)msg.op, src_ip, dst_ip, msg.stamp, msg.data, &new_msg);

        #ifdef WRITE_LOG
            write_log(TORUS_NODE_LOG, buf);
        #endif
		forward_message(new_msg, 0);
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

int rtree_split(const char *dst_ip, double plow[], double phigh[]) {

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
	socketfd = new_client_socket(dst_ip, DATA_PORT);
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
	socketfd = new_client_socket(dst_ip, DATA_PORT);
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
	socketfd = new_client_socket(dst_ip, DATA_PORT);
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
	socketfd = new_client_socket(dst_ip, DATA_PORT);
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
    torus_node *new_node = append_torus_node(d);

    // reset the regions(both current node and new append torus node
    int i;
    for (i = 0; i < MAX_DIM_NUM; i++) {
        new_node->info.region[i].low = low[i];
        new_node->info.region[i].high = high[i];
    }
    //TODO update the_torus region
    the_torus->info.region[d].low = (the_torus->info.region[d].low + the_torus->info.region[d].high) * 0.5; 

    
    // Step 2: update neighbor information
    clock_gettime(CLOCK_REALTIME, &start);

    //copy torus partitions into buf(if necessary)
    size_t cpy_len = 0;
    char buf[DATA_SIZE];
    memcpy(buf, (void *)&the_partition, sizeof(struct torus_partitions));
    cpy_len += sizeof(struct torus_partitions);

    // copy the torus node's neighbors info 
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
    if(query.op == RTREE_INSERT) {
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
    } else if(query.op == RTREE_DELETE) {
        if(FALSE == rtree_delete(query.data_id, plow, phigh, the_torus_rtree)) {
            return FALSE;
        }
    } else if(query.op == RTREE_QUERY) {
        if(FALSE == rtree_query(plow, phigh, the_torus_rtree)) {
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
    query.op = RTREE_QUERY;

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
                    socketfd = new_client_socket(dst_ip, LISTEN_PORT);
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
        write_log(RESULT_LOG, "here 1\n");
        int traj_exist = 0;
		if (g_TrajList.find(pt->p_tid) == g_TrajList.end()) {
            write_log(RESULT_LOG, "here 2\n");

			vector<OctPoint*> pt_vector;
			double range[3] = { 1.0, 1.0, 1.0 };  //需要定义一下，以这个点为中心的一个范围
			double low[3], high[3];
			for (int i = 0; i < 3; i++) {
				low[i] = pt->p_xyz[i] - range[i];
				high[i] = pt->p_xyz[i] + range[i];
			}
			//benjin rangequery
            traj_range_query(low, high, pt->p_tid, pt, pt_vector);//得到包含与其相邻点的vector  ToDo Benjin
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
			g_PtList.find(tmp->t_tail)->second->next = pt->p_id;
			pt->pre = tmp->t_tail;
			tmp->t_tail = pt->p_id;
            root->nodeInsert(pt);
            g_ptCount++;
            write_log(RESULT_LOG, "here 3\n");
		}
	} else {
		cout << "point not in this oct-tree!" << endl;
	}
    return TRUE;
}

int operate_oct_tree(struct query_struct query) {
    if(the_torus_rtree == NULL) {
        #ifdef WRITE_LOG
            write_log(TORUS_NODE_LOG, "torus oct tree didn't create.\n");
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
    if(query.op == RTREE_INSERT) {
        pthread_mutex_lock(&mutex);
		OctPoint *point = new OctPoint(query.data_id, -1, plow, query.trajectory_id, NONE, NONE);//后继指针都为空
		if(the_torus_oct_tree->containPoint(point)){
            oct_tree_insert(point);
            printNodes();
            write_log(RESULT_LOG, "here 4\n");
            //char buffer[1024];
            //memset(buffer, 0, 1024);
            //sprintf(buffer, "oct tree point %d %d %lf %lf %lf\n", point->p_id, point->p_tid, point->p_xyz[0], point->p_xyz[1], point->p_xyz[2]);
            //write_log(RESULT_LOG, buffer);
            if(g_NodeList.find(1)->second->n_ptCount >= (int)the_torus->info.capacity) {
                torus_split();
            }
            /*static int count = 1;
            time_t start;
            start = time(NULL);
            char buf[30];
            memset(buf, 0, 30);
            snprintf(buf, 30, "%ld %d\n", start, count);
            count++;
            write_log(RESULT_LOG, buf);*/
		}
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

    printPoints();

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

    // TODO set max_n_id to oct tree
    g_nodeCount = max_n_id + 1;
    g_ptCount = g_NodeList.find(1)->second->n_ptCount;

    clock_gettime(CLOCK_REALTIME, &end);
    elasped = get_elasped_time(start, end);

    char buffer[1024];
    memset(buffer, 0, 1024);
    sprintf(buffer, "receive split oct tree nodes spend %f ms\n", (double) elasped/ 1000000.0);
    write_log(RESULT_LOG, buffer);

    printNodes();

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

    printTrajs();

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
    int count;

    // get forward count from msg
	memcpy(&count, msg.data, sizeof(int));
    cpy_len += sizeof(int);
    // increase count and write back to msg
	count++;
	memcpy(msg.data, &count, sizeof(int));

    // get query string from msg
    struct query_struct query;
    memcpy(&query, msg.data + cpy_len, sizeof(struct query_struct));


	request *req_ptr = find_request(req_list, stamp);

    //struct message new_msg;

	if (NULL == req_ptr) {
        //TODO how can I delete the shit method to judge whether 
        //the request has been handled or not
        if(query.op == RTREE_QUERY) {
            req_ptr = insert_request(req_list, stamp);
        }

		if (1 == overlaps(query.intval, the_torus->info.region)) {

			// only for test
            /*
             * when recieve query and search torus node finished
             * record curent time and send to collect-result node
             */
            /*struct timespec query_end;
            clock_gettime(CLOCK_REALTIME, &query_end);
            new_msg.op = RECEIVE_RESULT;
            strncpy(new_msg.src_ip, msg.src_ip, IP_ADDR_LENGTH);
            strncpy(new_msg.dst_ip, result_ip, IP_ADDR_LENGTH);
            strncpy(new_msg.stamp, stamp, STAMP_SIZE);
            cpy_len = 0;
            memcpy(new_msg.data, &count, sizeof(int));
            cpy_len += sizeof(int);
            memcpy(new_msg.data + cpy_len, &query, sizeof(struct query_struct));
            cpy_len += sizeof(struct query_struct);
            memcpy(new_msg.data + cpy_len, &query_end, sizeof(struct timespec));
            forward_message(new_msg, 0);*/

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
                sprintf(buf + len, "\n%s:search torus success. %d\n\n", msg.dst_ip,
                        count);
                write_log(CTRL_NODE_LOG, buf);
            #endif

            if(query.op == RTREE_QUERY) {
                for (i = 0; i < MAX_DIM_NUM; i++) {
                    forward_search(query.intval, msg, i);
                }
            }

			// do rtree operation 
            //operate_rtree(query);
            
			// do oct tree operation 
            operate_oct_tree(query);

		} else {
            //TODO need delete the line below in true env. 
            if(query.op == RTREE_QUERY) {
                for (i = 0; i < MAX_DIM_NUM; i++) {
                    if (interval_overlap(query.intval[i], the_torus->info.region[i]) != 0) {
                        forward_search(query.intval, msg, i);
                        break;
                    }
                }
            }
		}
	}
	/*} else {
	 req_ptr->receive_num--;
	 if (req_ptr->receive_num == 0) {
	 remove_request(req_list, stamp);
	 }*/
	return TRUE;
}

int do_query_torus_cluster(struct message msg) {
	int i;
	char src_ip[IP_ADDR_LENGTH], dst_ip[IP_ADDR_LENGTH];
	char stamp[STAMP_SIZE];

    // get query from message
    struct query_struct query;

    size_t cpy_len = 0;
    int count;
	memcpy((void *)&count, msg.data, sizeof(int));
    cpy_len += sizeof(int);

	memcpy((void *) &query, msg.data + cpy_len, sizeof(struct query_struct));
    cpy_len += sizeof(struct query_struct);

	message new_msg;

    //get current skip list node  ip address
	get_node_ip(the_skip_list->header->leader, src_ip);

	skip_list_node *sln_ptr;
	sln_ptr = the_skip_list->header;

	if (0 == interval_overlap(sln_ptr->leader.region[2], query.intval[2])) { // node searched
		if (FALSE == gen_request_stamp(stamp)) {
			return FALSE;
		}


		// only for test
        /*
         * when recieve query and search skip list node finished
         * record curent time and send to collect-result node
         */
        /*struct timespec query_start;
        clock_gettime(CLOCK_REALTIME, &query_start);
        new_msg.op = RECEIVE_QUERY;
        strncpy(new_msg.src_ip, msg.src_ip, IP_ADDR_LENGTH);
        strncpy(new_msg.dst_ip, result_ip, IP_ADDR_LENGTH);
        strncpy(new_msg.stamp, stamp, STAMP_SIZE);
        cpy_len = 0;
        memcpy(new_msg.data, &count, sizeof(int));
        cpy_len += sizeof(int);
        memcpy(new_msg.data + cpy_len, &query, sizeof(struct query_struct));
        cpy_len += sizeof(query_struct);
        memcpy(new_msg.data + cpy_len, &query_start, sizeof(struct timespec));
		forward_message(new_msg, 0);*/
        // end test

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

            write_log(TORUS_NODE_LOG,
                    "search skip list success! turn to search torus\n");
        #endif

		// turn to torus layer
        fill_message((OP)QUERY_TORUS_NODE, src_ip, msg.dst_ip, stamp, msg.data, &new_msg);
		do_query_torus_node(new_msg);

        if(query.op == RTREE_QUERY) {

            //decide whether forward message to it's forward and backward
            if (strcmp(msg.stamp, "") == 0) {
                if ((sln_ptr->level[0].forward != NULL)
                        && (interval_overlap(
                                sln_ptr->level[0].forward->leader.region[2],
                                query.intval[2]) == 0)) {
                    get_node_ip(sln_ptr->level[0].forward->leader, dst_ip);
                    fill_message((OP)msg.op, src_ip, dst_ip, "forward", msg.data,
                            &new_msg);
                    forward_message(new_msg, 0);
                }

                if ((sln_ptr->level[0].backward != NULL)
                        && (interval_overlap(
                                sln_ptr->level[0].backward->leader.region[2],
                                query.intval[2]) == 0)) {
                    get_node_ip(sln_ptr->level[0].backward->leader, dst_ip);
                    fill_message((OP)msg.op, src_ip, dst_ip, "backward", msg.data,
                            &new_msg);
                    forward_message(new_msg, 0);
                }
            } else if (strcmp(msg.stamp, "forward") == 0) {
                if ((sln_ptr->level[0].forward != NULL)
                        && (interval_overlap(
                                sln_ptr->level[0].forward->leader.region[2],
                                query.intval[2]) == 0)) {
                    get_node_ip(sln_ptr->level[0].forward->leader, dst_ip);
                    fill_message((OP)msg.op, src_ip, dst_ip, "forward", msg.data,
                            &new_msg);
                    forward_message(new_msg, 0);
                }
            } else {
                if ((sln_ptr->level[0].backward != NULL)
                        && (interval_overlap(
                                sln_ptr->level[0].backward->leader.region[2],
                                query.intval[2]) == 0)) {
                    get_node_ip(sln_ptr->level[0].backward->leader, dst_ip);
                    fill_message((OP)msg.op, src_ip, dst_ip, "backward", msg.data,
                            &new_msg);
                    forward_message(new_msg, 0);
                }
            }
        }

	} else if (-1 == interval_overlap(sln_ptr->leader.region[2], query.intval[2])) { // node is on the forward of skip list
		int visit_forward = 0;
		for (i = the_skip_list->level; i >= 0; --i) {
			if ((sln_ptr->level[i].forward != NULL)
					&& (interval_overlap(
							sln_ptr->level[i].forward->leader.region[2],
							query.intval[2]) <= 0)) {

				get_node_ip(sln_ptr->level[i].forward->leader, dst_ip);
				fill_message((OP)msg.op, src_ip, dst_ip, "forward", msg.data,
						&new_msg);
				forward_message(new_msg, 0);
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
			if ((sln_ptr->level[i].backward != NULL)
					&& (interval_overlap(
							sln_ptr->level[i].backward->leader.region[2],
							query.intval[2]) >= 0)) {

				get_node_ip(sln_ptr->level[i].backward->leader, dst_ip);
				fill_message((OP)msg.op, src_ip, dst_ip, "backward", msg.data,
						&new_msg);
				forward_message(new_msg, 0);
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

//return elasped time from start to finish
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

int do_receive_data(int socketfd) {

    char file_name[30];
    sprintf(file_name, "./files/receive_file");

    FILE *fp = fopen(file_name, "a");
    if(fp == NULL) {
        printf("open receive_file to write failed.\n");
        return FALSE;
    }

    struct timespec start, end;
    double elasped = 0L;

    char buf[SOCKET_BUF_SIZE];
    memset(buf, 0, SOCKET_BUF_SIZE);
    int length = 0, loop = 1;

    while(loop) {
        clock_gettime(CLOCK_REALTIME, &start);
        length = recv(socketfd, buf, SOCKET_BUF_SIZE, 0);
        clock_gettime(CLOCK_REALTIME, &end);
        elasped += get_elasped_time(start, end);
        if(length == 0) {
            //write_log(RESULT_LOG, "length 0\n");
            loop = 0;
        } else if(length < 0) {
            if(errno == EAGAIN) {
                ;
                //write_log(RESULT_LOG, "recv eagain\n");
                //regisiter event
                //epoll_event event;
                //event.data.fd = socketfd;
                //event.events = EPOLLIN | EPOLLONESHOT;
                //epoll_ctl(epfd, EPOLL_CTL_MOD, socketfd, &event);
                //is_eagain = 1;
                //loop = 0;
            } else {
                //write_log(RESULT_LOG, "errno occur\n");
                ;
            }
            continue;
        } else {
            int block_len = fwrite(buf, sizeof(char), length, fp);
            if(block_len < length){
                printf("write file failed.\n");
                break;
            }
            memset(buf, 0, SOCKET_BUF_SIZE);
        }
    }
    fwrite("---------\n", sizeof(char), 10, fp);
    fclose(fp);
    close(socketfd);

    char buffer[1024];
    memset(buffer, 0, 1024);
    sprintf(buffer, "receive spend %f ms\n", (double) elasped/ 1000000.0);
    write_log(RESULT_LOG, buffer);

    return TRUE;
}

int do_performance_test(struct message msg) {
    static int count = 1;
    //pthread_mutex_lock(&mutex);
    time_t start;
    start = time(NULL);
    char buf[30];
    memset(buf, 0, 30);
    snprintf(buf, 30, "%ld %d\n", start, count);
    count++;
    write_log(RESULT_LOG, buf);
    //pthread_mutex_unlock(&mutex);

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

		/*reply_msg.op = (OP)msg.op;
		reply_msg.reply_code = (REPLY_CODE)reply_code;
		strncpy(reply_msg.stamp, msg.stamp, STAMP_SIZE);

		if (FALSE == send_reply(socketfd, reply_msg)) {
			// TODO handle send reply failed
			return FALSE;
		}*/
		print_torus_node(*the_torus);
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
		print_skip_list_node(the_skip_list);
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

		do_query_torus_cluster(msg);
		break;

	case UPDATE_FORWARD:
        #ifdef WRITE_LOG
            write_log(TORUS_NODE_LOG, "receive request update skip list node's forward field.\n");
        #endif

		do_update_forward(msg);
		print_skip_list_node(the_skip_list);
		break;

	case UPDATE_BACKWARD:
        #ifdef WRITE_LOG
            write_log(TORUS_NODE_LOG, "receive request update skip list node's backward field.\n");
        #endif

		do_update_backward(msg);
		print_skip_list_node(the_skip_list);
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

	case RECEIVE_QUERY:
        #ifdef WRITE_LOG
            write_log(TORUS_NODE_LOG, "receive request receive query.\n");
        #endif

		//do_receive_query(msg);
		break;

	case RECEIVE_RESULT:
        #ifdef WRITE_LOG
            write_log(TORUS_NODE_LOG, "receive request receive result.\n");
        #endif

		//do_receive_result(msg);
		break;

    case RECEIVE_DATA:
        #ifdef WRITE_LOG
            write_log(TORUS_NODE_LOG, "receive request receive data.\n");
        #endif
        do_receive_data(conn->socketfd);
        break;
    case PERFORMANCE_TEST:
        #ifdef WRITE_LOG
            write_log(TORUS_NODE_LOG, "receive request performance test.\n");
        #endif

        do_performance_test(msg);
        break;

    case RELOAD_RTREE:
        #ifdef WRITE_LOG
            write_log(TORUS_NODE_LOG, "receive request reload rtree data.\n");
        #endif

        // reply to client for RTREE_LOAD requet is necessary because 
        // the client should receive this reply before send more data
        // if not, the following data from client will send to epoll
		/*reply_msg.op = (OP)msg.op;
		reply_msg.reply_code = (REPLY_CODE)reply_code;
		strncpy(reply_msg.stamp, msg.stamp, STAMP_SIZE);

		if (FALSE == send_reply(conn->socketfd, reply_msg)) {
			// TODO handle send reply failed
			return FALSE;
		}*/

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
            //process message
            //pthread_mutex_lock(&mutex);
            process_message(conn, msg);
            //pthread_mutex_unlock(&mutex);
            beg = beg + msg_size;
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
            /*char buf[1024];
            memset(buf, 0, 1024);
            sprintf(buf, "%s\n",strerror(errno));
            write_log(RESULT_LOG, buf);*/
            return FALSE;
        }
    }
    return TRUE;
}

void *listen_epoll(void *args) {
	// listen epoll
	int epfd;
	struct epoll_event ev, event;
	epfd = epoll_create(MAX_EVENTS);
	ev.data.fd = server_socket;
	ev.events = EPOLLIN;
	epoll_ctl(epfd, EPOLL_CTL_ADD, server_socket, &ev);

	int nfds;
    int conn_socket;
    //worker epoll index
    int index = 0;

	while(should_run) {
		nfds = epoll_wait(epfd, &event, 1, -1);
		//for(i = 0; i < nfds; i++) {
        if(nfds > 0) {
            while((conn_socket = accept_connection(server_socket)) > 0) {
                g_conn_table[conn_socket].used = 1;

                set_nonblocking(conn_socket);

                ev.events = EPOLLIN | EPOLLONESHOT;
                ev.data.fd = conn_socket;

                /* register conn_socket to worker-pool's 
                 * epoll, not the listen epoll*/

                g_conn_table[conn_socket].index = index; 
                g_conn_table[conn_socket].socketfd = conn_socket; 
                epoll_ctl(worker_epfd[index], EPOLL_CTL_ADD, conn_socket, &ev);
                index = (index + 1) % EPOLL_NUM;
            }
		} 
	}

    close(epfd);
    return NULL;
}

void close_connection(connection_t conn) {
    if(conn->used != 0) {
        struct epoll_event ev;

        conn->used = 0;
        conn->roff = 0;
        epoll_ctl(worker_epfd[conn->index], EPOLL_CTL_DEL, conn->socketfd, &ev);
        close(conn->socketfd);
    }
}

void *work_epoll(void *args){
    int epfd = *(int *)args;

	struct epoll_event ev, event; //events[MAX_EVENTS];

    int nfds;
    int conn_socket;

	while(should_run) {
		nfds = epoll_wait(epfd, &event, 1, -1);
		//for(i = 0; i < nfds; i++) {
        if(nfds > 0) {
            if (event.events && EPOLLIN) {
				conn_socket = event.data.fd;

                connection_t conn = &g_conn_table[conn_socket];

                if(FALSE == handle_read_event(conn)) {
                    close_connection(conn);
                    continue;
                }

                if(conn->used != 0) {
                    ev.events = EPOLLIN | EPOLLONESHOT;
                    ev.data.fd = conn_socket;
                    epoll_ctl(epfd, EPOLL_CTL_MOD, conn->socketfd, &ev);
                }
			}
		}
	}

    return NULL;
}

void *data_transform(void *args) {
	// listen epoll
	int epfd;
	struct epoll_event ev, event;
	epfd = epoll_create(MAX_EVENTS);
	ev.data.fd = data_socket;
	ev.events = EPOLLIN | EPOLLET;
	epoll_ctl(epfd, EPOLL_CTL_ADD, data_socket, &ev);

	int nfds;
    int conn_socket;
    struct message msg;

	while(should_run) {
		nfds = epoll_wait(epfd, &event, 1, -1);
        if(nfds > 0) {
            if(event.data.fd == data_socket) {
                while((conn_socket = accept_connection(data_socket)) > 0) {
                    g_conn_table[conn_socket].used = 1;

                    set_nonblocking(conn_socket);
                    ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
                    ev.data.fd = conn_socket;

                    g_conn_table[conn_socket].socketfd = conn_socket; 
                    epoll_ctl(epfd, EPOLL_CTL_ADD, conn_socket, &ev);
                }
            } else if(event.events && EPOLLIN) { 
				conn_socket = event.data.fd;
				memset(&msg, 0, sizeof(struct message));

				// receive message through the conn_socket
				if (TRUE == receive_message(conn_socket, &msg)) {
					process_message(&g_conn_table[conn_socket], msg);
				} else {
					//  TODO: handle receive message failed
					printf("receive message failed.\n");
				}
            }
		} 
	}

    close(epfd);
    return NULL;
}

int main(int argc, char **argv) {

	printf("start torus node.\n");
	write_log(TORUS_NODE_LOG, "start torus node.\n");

	int i;
	should_run = 1;

	// new a torus node
	the_torus = NULL; // = new_torus_node();

	the_skip_list = NULL; //*new_skip_list_node();

	// create a new request list
	req_list = new_request();

	// create rtree
    the_torus_rtree = NULL;
	the_torus_rtree = rtree_create();
    if(the_torus_rtree == NULL){
        write_log(TORUS_NODE_LOG, "create rtree failed.\n");
        exit(1);
    }
	printf("create rtree.\n");
	write_log(TORUS_NODE_LOG, "create rtree.\n");

    // start oct_tree
    // do nothing, all structure are stored in global hash_maps in OctTree;
	the_torus_oct_tree = NULL;

	server_socket = new_server(LISTEN_PORT);
	if (server_socket == FALSE) {
        write_log(TORUS_NODE_LOG, "start server failed.\n");
        exit(1);
	}
	printf("start server.\n");
	write_log(TORUS_NODE_LOG, "start server.\n");

	data_socket = new_server(DATA_PORT);
	if (data_socket == FALSE) {
        write_log(TORUS_NODE_LOG, "start data server failed.\n");
        exit(1);
	}
	printf("start data server.\n");
	write_log(TORUS_NODE_LOG, "start data server.\n");


    // set socket fd reused
    //int reuse = 1;
    //setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

	// set server socket nonblocking
	set_nonblocking(server_socket);
	set_nonblocking(data_socket);


    // init mutex;
    pthread_mutex_init(&mutex, NULL);

    //create EPOLL_NUM different epoll fd for workers
    for (i = 0; i < EPOLL_NUM; i++) {
        worker_epfd[i] = epoll_create(MAX_EVENTS);
    }

    // create listen thread for epoll listerner
    pthread_create(&listener_thread, NULL, listen_epoll, NULL);

    // create worker thread to handle ready socket fds
    for(i = 0; i < EPOLL_NUM; i++) {
        int j; 
        for(j = 0; j < WORKER_PER_GROUP; j++) {
            pthread_create(worker_thread + (i * WORKER_PER_GROUP + j), NULL, work_epoll, worker_epfd + i);
        }
    }

    pthread_t data_thread;
    pthread_create(&data_thread, NULL, data_transform, NULL);
    pthread_join(data_thread, NULL);

    for (i = 0; i < WORKER_NUM; i++) {
        pthread_join(worker_thread[i], NULL);
    }

    pthread_join(listener_thread, NULL);

    // close worker epoll fds
    for (i = 0; i < EPOLL_NUM; i++) {
        close(worker_epfd[i]);
    }

    close(server_socket);

	return 0;
}

