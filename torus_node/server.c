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

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/time.h>
#include<time.h>
#include<sys/epoll.h>
#include<pthread.h>
#include<errno.h>


//define torus server 
/*****************************************************************************/

torus_node *the_torus;
struct torus_partitions the_partition;

ISpatialIndex* the_torus_rtree;

// mark torus node is active or not 
int should_run;

struct skip_list *the_skip_list;

//pthread_mutex_t mutex;
//pthread_cond_t condition;

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

int do_update_torus(struct message msg) {
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
			forward_message(msg, 1);
		}

		if (backward) {
			get_node_ip(backward->leader, dst_ip);
			strncpy(msg.dst_ip, dst_ip, IP_ADDR_LENGTH);
			strncpy(msg.data, "b", DATA_SIZE);
			forward_message(msg, 1);
		}

	} else if (strcmp(msg.data, "f") == 0) {
		if (forward) {
			get_node_ip(forward->leader, dst_ip);
			strncpy(msg.dst_ip, dst_ip, IP_ADDR_LENGTH);
			forward_message(msg, 1);
		}

	} else {
		if (backward) {
			get_node_ip(backward->leader, dst_ip);
			strncpy(msg.dst_ip, dst_ip, IP_ADDR_LENGTH);
			forward_message(msg, 1);
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
	memcpy(&f_node, (void*) (msg.data + cpy_len), sizeof(node_info));
    cpy_len += sizeof(node_info);

	memcpy(&b_node, (void*) (msg.data + cpy_len), sizeof(node_info));
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

	int lower_overlap = interval_overlap(intval[d], lower_neighbor->dims[d]);
	int upper_overlap = interval_overlap(intval[d], upper_neighbor->dims[d]);

	// choose forward neighbor if query overlap one neighbor at most
	if ((lower_overlap != 0) || (upper_overlap != 0)) {

		//current torus node is the first node on dimension d
		if ((the_torus->info.dims[d].low < lower_neighbor->dims[d].low)
				&& (the_torus->info.dims[d].low
						< upper_neighbor->dims[d].low)) {
			if (get_distance(intval[d], lower_neighbor->dims[d])
					< get_distance(intval[d], upper_neighbor->dims[d])) {
				upper_neighbor = lower_neighbor;
			}
			lower_neighbor = NULL;

		} else if ((the_torus->info.dims[d].high
				> lower_neighbor->dims[d].high)
				&& (the_torus->info.dims[d].high
						> upper_neighbor->dims[d].high)) {
			if (get_distance(intval[d], upper_neighbor->dims[d])
					< get_distance(intval[d], lower_neighbor->dims[d])) {
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

	set_node_ip(info_ptr, "172.16.0.100");
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
            plow[j] = (double)the_torus->info.dims[j].low;
            phigh[j] = (double)the_torus->info.dims[j].high;
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

int rtree_split(char *dst_ip, double plow[], double phigh[]) {

    char buffer[1024];
    struct timespec start, end;
    double elasped = 0L;
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

	int socketfd;

	socketfd = new_client_socket(dst_ip);
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
    send_message(socketfd, msg);

    char buf[SOCKET_BUF_SIZE];
    memset(buf, 0, SOCKET_BUF_SIZE);
    size_t cpy_len = sizeof(int);
    int total_len = 0;
    int count = 0;
    Region region;

    for(it = v.begin(); it != v.end(); it++) {
        IShape *pS;
        (*it)->getShape(&pS);
        id_type id = (*it)->getIdentifier();

        // delete data from the_torus_rtree
        //if(the_torus_rtree->deleteData(*pS, id) == false) {
        //    return FALSE;
        //}

        pS->getMBR(region);
        byte *r;
        uint32_t r_len;
        region.storeToByteArray(&r, r_len);

        // data num
        count++;
        memcpy(buf + 0, &count, sizeof(int));

        memcpy(buf + cpy_len, &id, sizeof(id_type));
        cpy_len += sizeof(id_type);

        memcpy(buf + cpy_len, &r_len, sizeof(uint32_t));
        cpy_len += sizeof(uint32_t);

        memcpy(buf + cpy_len, r, r_len);
        cpy_len += r_len;

        delete[] r;
        delete pS;

        if(cpy_len + 100 < SOCKET_BUF_SIZE) {
            continue;
        } else {
            send(socketfd, (void *) buf, SOCKET_BUF_SIZE, 0);
            //if (FALSE == send_data(RELOAD_RTREE, dst_ip, buf, cpy_len)) {
            //    return FALSE;
            //}

            total_len += cpy_len;
            count = 0;
            cpy_len = sizeof(int);
            memset(buf, 0, SOCKET_BUF_SIZE);
        }
    }
    if(cpy_len > sizeof(int)) {
        send(socketfd, (void *) buf, SOCKET_BUF_SIZE, 0);
    }
	close(socketfd);

    total_len += cpy_len;
    //if (FALSE == send_data(RELOAD_RTREE, dst_ip, buf, cpy_len)) {
    //    return FALSE;
    //}
    memset(buffer, 0, 1024);
    sprintf(buffer, "total send %f k\n", (double) total_len/ 1000.0);
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

int torus_split() {
    // Step 1: append a new torus node and reset regions
    // get the optimal split dimension
    char buffer[1024];

    struct timespec start, end;
    double elasped = 0L;
    clock_gettime(CLOCK_REALTIME, &start);
    int d = get_split_dimension();
    clock_gettime(CLOCK_REALTIME, &end);
    elasped = get_elasped_time(start, end);
    memset(buffer, 0, 1024);
    sprintf(buffer, "get_split_dimension spend %f ms\n", (double) elasped/ 1000000.0);
    write_log(RESULT_LOG, buffer);

    // append a new torus node
    torus_node *new_node = append_torus_node(d);

    // reset the regions(both current node and new append torus node
    int i;
    for (i = 0; i < MAX_DIM_NUM; i++) {
        new_node->info.dims[i] = the_torus->info.dims[i];
    }
    the_torus->info.dims[d].low = (the_torus->info.dims[d].low + the_torus->info.dims[d].high) * 0.5; 
    new_node->info.dims[d].high = (new_node->info.dims[d].low + new_node->info.dims[d].high) * 0.5;

    
    // Step 2: update neighbor information
    clock_gettime(CLOCK_REALTIME, &start);

    //copy torus partitions into buf(if necessary)
    size_t cpy_len = 0;
    char buf[DATA_SIZE];
    memcpy(buf, (void *)&the_partition, sizeof(struct torus_partitions));
    cpy_len += sizeof(struct torus_partitions);
    // copy neighbors info into buf 
    // then send to new torus node
    memcpy(buf + cpy_len,(void *) &new_node->info, sizeof(node_info));
    cpy_len += sizeof(node_info);
    for(i = 0; i < DIRECTIONS; i++) {
        int num = get_neighbors_num_d(the_torus, i);
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
        // d is the dimension, (include 2 directions)
        // 2*d represent torus node's lower direction
        // 2*d + 1 represent torus node's upper direction 
        if(i == 2 * d + 1) {
            memcpy(buf + cpy_len, &the_torus->info, sizeof(node_info));
            cpy_len += sizeof(node_info);
        }
    }
    char dst_ip[IP_ADDR_LENGTH];
    memset(dst_ip, 0, IP_ADDR_LENGTH);
    get_node_ip(new_node->info, dst_ip);

    // send to dst_ip
    if (FALSE == send_data(UPDATE_TORUS, dst_ip, buf, cpy_len)) {
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

    // Step 3: copy rtree data to new torus node and delete local rtree data
    double nlow[MAX_DIM_NUM], nhigh[MAX_DIM_NUM], plow[MAX_DIM_NUM], phigh[MAX_DIM_NUM];
    for (i = 0; i < MAX_DIM_NUM; i++) {
        nlow[i] = new_node->info.dims[i].low; 
        nhigh[i] = new_node->info.dims[i].high;
        plow[i] = the_torus->info.dims[i].low;
        phigh[i] = the_torus->info.dims[i].high;
    }

    clock_gettime(CLOCK_REALTIME, &start);
    rtree_split(dst_ip, nlow, nhigh);
    clock_gettime(CLOCK_REALTIME, &end);
    elasped = get_elasped_time(start, end);
    memset(buffer, 0, 1024);
    sprintf(buffer, "send split rtree spend %f ms\n", (double) elasped/ 1000000.0);
    write_log(RESULT_LOG, buffer);


    clock_gettime(CLOCK_REALTIME, &start);
    rtree_recreate(plow, phigh);
    clock_gettime(CLOCK_REALTIME, &end);
    elasped = get_elasped_time(start, end);

    memset(buffer, 0, 1024);
    sprintf(buffer, "rtree recreate rtree spend %f ms\n", (double) elasped/ 1000000.0);
    write_log(RESULT_LOG, buffer);

    return TRUE;
}

int operate_rtree(int op, int id, struct interval intval[]) {

    if(the_torus_rtree == NULL) {
        #ifdef WRITE_LOG
            write_log(TORUS_NODE_LOG, "torus rtree didn't create.\n");
        #endif
        return FALSE;
    }

    int i;
    double plow[MAX_DIM_NUM], phigh[MAX_DIM_NUM];

    for (i = 0; i < MAX_DIM_NUM; i++) {
        plow[i] = (double)intval[i].low;
        phigh[i] = (double)intval[i].high;
    }

    // rtree operation 
    if(op == RTREE_INSERT) {
        if(FALSE == rtree_insert(id, plow, phigh, the_torus_rtree)) {
            return FALSE;
        }
        size_t c = rtree_get_utilization(the_torus_rtree);
        if(c >= the_torus->info.capacity){
            torus_split();
        }
    } else if(op == RTREE_DELETE) {
        if(FALSE == rtree_delete(id, plow, phigh, the_torus_rtree)) {
            return FALSE;
        }
    } else if(op == RTREE_QUERY) {
        if(FALSE == rtree_query(plow, phigh, the_torus_rtree)) {
            return FALSE;
        }
    }
    return TRUE;
}

int do_rtree_load_data(int socketfd, struct message msg){
    struct timespec start, end;
    double elasped = 0L;
    clock_gettime(CLOCK_REALTIME, &start);

    char buffer[1024];
    char buf[SOCKET_BUF_SIZE];
    memset(buf, 0, SOCKET_BUF_SIZE);
    int length = 0, loop = 1;

    std::vector<SpatialIndex::IData*> v;

    int dlen, dnum;
    id_type id;
    byte bf[SOCKET_BUF_SIZE];
    size_t cpy_len = 0;
    Region region;

    while(loop) {
        length = recv(socketfd, buf, SOCKET_BUF_SIZE, 0);
        if(length == 0) {
            loop = 0;
        } else if(length < 0) {
            continue;
        } else {
            cpy_len = 0;
            memcpy(&dnum, buf, sizeof(int));
            cpy_len += sizeof(int);

            while(dnum--){
                memcpy(&id, buf + cpy_len, sizeof(id_type));
                cpy_len += sizeof(id_type);

                memcpy(&dlen, buf + cpy_len, sizeof(uint32_t));
                cpy_len += sizeof(uint32_t);

                memset(bf, 0, SOCKET_BUF_SIZE);
                memcpy(bf, buf + cpy_len, dlen);
                cpy_len += dlen;

                region.loadFromByteArray(bf);

                SpatialIndex::IData *data = dynamic_cast<SpatialIndex::IData*>(new Data(sizeof(double), reinterpret_cast<byte*>(region.m_pLow), region, id));
                v.push_back(data);
            }
            usleep(20);
            memset(buf, 0, SOCKET_BUF_SIZE);
        }
    }
	close(socketfd);

    clock_gettime(CLOCK_REALTIME, &end);
    elasped = get_elasped_time(start, end);
    memset(buffer, 0, 1024);
    sprintf(buffer, "receive split data%f ms\n", (double) elasped/ 1000000.0);
    write_log(RESULT_LOG, buffer);

    memset(buffer, 0, 1024);
    sprintf(buffer, "finish get split data\n");
    write_log(RESULT_LOG, buffer);

    //bulkload rtree;
    the_torus_rtree = rtree_bulkload(v);

    std::vector<SpatialIndex::IData*>::iterator it;
    for (it = v.begin(); it != v.end(); it++) {
        delete *it;
    } 

    size_t c = rtree_get_utilization(the_torus_rtree);
    memset(buffer, 0, 1024);
    sprintf(buffer, "%lu\n", c);
    write_log(RESULT_LOG, buffer);
    return TRUE;
}

int do_search_torus_node(struct message msg) {
	int i;
    //char src_ip[IP_ADDR_LENGTH], dst_ip[IP_ADDR_LENGTH];
	char stamp[STAMP_SIZE];

    // get query from message
	int count, op, id, cpy_len = 0;
	struct interval intval[MAX_DIM_NUM];

    // get requst stamp from msg
	strncpy(stamp, msg.stamp, STAMP_SIZE);

    // get forward count from msg
	memcpy(&count, msg.data, sizeof(int));
    cpy_len += sizeof(int);

    // increase count and write back to msg
	count++;
	memcpy(msg.data, &count, sizeof(int));

    // get query string from msg
    // query op
	memcpy(&op, msg.data + cpy_len, sizeof(int));
    cpy_len += sizeof(int);
    // query id
	memcpy(&id, msg.data + cpy_len, sizeof(int));
    cpy_len += sizeof(int);
    // query string
	memcpy(intval, msg.data + cpy_len, sizeof(struct interval) * MAX_DIM_NUM);

	request *req_ptr = find_request(req_list, stamp);

    //struct message new_msg;

	if (NULL == req_ptr) {
        if(op == RTREE_QUERY) {
            req_ptr = insert_request(req_list, stamp);
        }

		if (1 == overlaps(intval, the_torus->info.dims)) {

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
            memcpy(new_msg.data + cpy_len, intval, sizeof(struct interval) * MAX_DIM_NUM);
            cpy_len += sizeof(interval) * MAX_DIM_NUM;
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
                        len += sprintf(buf + len, "[%d, %d] ", intval[i].low,
                                intval[i].high);
                    #else
                        len += sprintf(buf + len, "[%lf, %lf] ", intval[i].low,
                                intval[i].high);
                    #endif
                }
                sprintf(buf + len, "\n");
                write_log(CTRL_NODE_LOG, buf);

                len = 0;
                len = sprintf(buf, "%s:", the_torus->info.ip);
                for (i = 0; i < MAX_DIM_NUM; ++i) {
                    #ifdef INT_DATA
                        len += sprintf(buf + len, "[%d, %d] ",
                                the_torus->info.dims[i].low,
                                the_torus->info.dims[i].high);
                    #else
                        len += sprintf(buf + len, "[%lf, %lf] ",
                                the_torus->info.dims[i].low,
                                the_torus->info.dims[i].high);
                    #endif
                }
                sprintf(buf + len, "\n%s:search torus success. %d\n\n", msg.dst_ip,
                        count);
                write_log(CTRL_NODE_LOG, buf);
            #endif

            if(op == RTREE_QUERY) {
                for (i = 0; i < MAX_DIM_NUM; i++) {
                    forward_search(intval, msg, i);
                }
            }

			// do rtree operation 
            operate_rtree(op, id, intval);

		} else {
			for (i = 0; i < MAX_DIM_NUM; i++) {
				if (interval_overlap(intval[i], the_torus->info.dims[i]) != 0) {
					forward_search(intval, msg, i);
					break;
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

int do_search_skip_list_node(struct message msg) {
	int i;
    size_t cpy_len = 0;
	char src_ip[IP_ADDR_LENGTH], dst_ip[IP_ADDR_LENGTH];
	char stamp[STAMP_SIZE];

    // get query from message
    int count, query_op, query_id;
	struct interval intval[MAX_DIM_NUM];

	memcpy((void *)&count, msg.data, sizeof(int));
    cpy_len += sizeof(int);

	memcpy((void *) &query_op, msg.data + cpy_len, sizeof(int));
    cpy_len += sizeof(int);

	memcpy((void *) &query_id, msg.data + cpy_len, sizeof(int));
    cpy_len += sizeof(int);

	memcpy(intval, msg.data + cpy_len, sizeof(struct interval) * MAX_DIM_NUM);
    cpy_len += sizeof(interval) * MAX_DIM_NUM;

	message new_msg;

    //get current skip list node  ip address
	get_node_ip(the_skip_list->header->leader, src_ip);

	skip_list_node *sln_ptr;
	sln_ptr = the_skip_list->header;

	if (0 == interval_overlap(sln_ptr->leader.dims[2], intval[2])) { // node searched
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
        memcpy(new_msg.data + cpy_len, intval, sizeof(struct interval) * MAX_DIM_NUM);
        cpy_len += sizeof(interval) * MAX_DIM_NUM;
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
                    len += sprintf(buf + len, "[%d, %d] ", intval[i].low,
                            intval[i].high);
                #else
                    len += sprintf(buf + len, "[%lf, %lf] ", intval[i].low,
                            intval[i].high);
                #endif
            }
            sprintf(buf + len, "\n");
            write_log(TORUS_NODE_LOG, buf);

            write_log(TORUS_NODE_LOG,
                    "search skip list success! turn to search torus\n");
        #endif

		// turn to torus layer
        fill_message((OP)SEARCH_TORUS_NODE, src_ip, msg.dst_ip, stamp, msg.data, &new_msg);
		do_search_torus_node(new_msg);

        if(query_op == RTREE_QUERY) {

            //decide whether forward message to it's forward and backward
            if (strcmp(msg.stamp, "") == 0) {
                if ((sln_ptr->level[0].forward != NULL)
                        && (interval_overlap(
                                sln_ptr->level[0].forward->leader.dims[2],
                                intval[2]) == 0)) {
                    get_node_ip(sln_ptr->level[0].forward->leader, dst_ip);
                    fill_message((OP)msg.op, src_ip, dst_ip, "forward", msg.data,
                            &new_msg);
                    forward_message(new_msg, 1);
                }

                if ((sln_ptr->level[0].backward != NULL)
                        && (interval_overlap(
                                sln_ptr->level[0].backward->leader.dims[2],
                                intval[2]) == 0)) {
                    get_node_ip(sln_ptr->level[0].backward->leader, dst_ip);
                    fill_message((OP)msg.op, src_ip, dst_ip, "backward", msg.data,
                            &new_msg);
                    forward_message(new_msg, 1);
                }
            } else if (strcmp(msg.stamp, "forward") == 0) {
                if ((sln_ptr->level[0].forward != NULL)
                        && (interval_overlap(
                                sln_ptr->level[0].forward->leader.dims[2],
                                intval[2]) == 0)) {
                    get_node_ip(sln_ptr->level[0].forward->leader, dst_ip);
                    fill_message((OP)msg.op, src_ip, dst_ip, "forward", msg.data,
                            &new_msg);
                    forward_message(new_msg, 1);
                }
            } else {
                if ((sln_ptr->level[0].backward != NULL)
                        && (interval_overlap(
                                sln_ptr->level[0].backward->leader.dims[2],
                                intval[2]) == 0)) {
                    get_node_ip(sln_ptr->level[0].backward->leader, dst_ip);
                    fill_message((OP)msg.op, src_ip, dst_ip, "backward", msg.data,
                            &new_msg);
                    forward_message(new_msg, 1);
                }
            }
        }

	} else if (-1 == interval_overlap(sln_ptr->leader.dims[2], intval[2])) { // node is on the forward of skip list
		int visit_forward = 0;
		for (i = the_skip_list->level; i >= 0; --i) {
			if ((sln_ptr->level[i].forward != NULL)
					&& (interval_overlap(
							sln_ptr->level[i].forward->leader.dims[2],
							intval[2]) <= 0)) {

				get_node_ip(sln_ptr->level[i].forward->leader, dst_ip);
				fill_message((OP)msg.op, src_ip, dst_ip, "forward", msg.data,
						&new_msg);
				forward_message(new_msg, 1);
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
							sln_ptr->level[i].backward->leader.dims[2],
							intval[2]) >= 0)) {

				get_node_ip(sln_ptr->level[i].backward->leader, dst_ip);
				fill_message((OP)msg.op, src_ip, dst_ip, "backward", msg.data,
						&new_msg);
				forward_message(new_msg, 1);
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

	interval intval[MAX_DIM_NUM];
	memcpy(stamp, msg.stamp, STAMP_SIZE);
	memcpy(ip, msg.src_ip, IP_ADDR_LENGTH);

	memcpy(&count, msg.data, sizeof(int));
    cpy_len += sizeof(int);
	memcpy(intval, msg.data + cpy_len, sizeof(struct interval) * MAX_DIM_NUM);
    cpy_len += sizeof(struct interval) * MAX_DIM_NUM;
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
            fprintf(fp, "[%d, %d] ", intval[i].low, intval[i].high);
        #else
            fprintf(fp, "[%lf, %lf] ", intval[i].low, intval[i].high);
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

	interval intval[MAX_DIM_NUM];
	memcpy(stamp, msg.stamp, STAMP_SIZE);
	memcpy(ip, msg.src_ip, IP_ADDR_LENGTH);

	memcpy(&count, msg.data, sizeof(int));
    cpy_len += sizeof(int);
	memcpy(intval, msg.data + cpy_len, sizeof(struct interval) * MAX_DIM_NUM);
    cpy_len += sizeof(struct interval) * MAX_DIM_NUM;
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
            fprintf(fp, "[%d, %d] ", intval[i].low, intval[i].high);
        #else
            fprintf(fp, "[%lf, %lf] ", intval[i].low, intval[i].high);
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

    char buf[SOCKET_BUF_SIZE];
    memset(buf, 0, SOCKET_BUF_SIZE);
    int length = 0, loop = 1;

    while(loop) {
        length = recv(socketfd, buf, SOCKET_BUF_SIZE, 0);
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
    return TRUE;
}

int do_performance_test(struct message msg) {
	struct timespec cur;
    clock_gettime(CLOCK_REALTIME, &cur);
    double start= 0.0;
    start = (double)(1000000L * cur.tv_sec + cur.tv_nsec/1000);
    char buf[30];
    memset(buf, 0, 30);
    snprintf(buf, 30, "%f\n", start);
    write_log(RESULT_LOG, buf);


    /*char buf[SOCKET_BUF_SIZE];
    memset(buf, 0, SOCKET_BUF_SIZE);

    if(strcmp(msg.data, "hello server") == 0){
        snprintf(buf, DATA_SIZE, "%s\n", msg.data);
        write_log(RESULT_LOG, buf);
        return TRUE;
    }*/

    return TRUE;
}

int process_message(int socketfd, struct message msg) {

	struct reply_message reply_msg;
	REPLY_CODE reply_code = SUCCESS;
	switch (msg.op) {

	case UPDATE_TORUS:
        #ifdef WRITE_LOG
            write_log(TORUS_NODE_LOG, "receive request update torus.\n");
        #endif

		if (FALSE == do_update_torus(msg)) {
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

		if (FALSE == send_reply(socketfd, reply_msg)) {
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

	case SEARCH_TORUS_NODE:
        #ifdef WRITE_LOG
            write_log(TORUS_NODE_LOG, "\nreceive request search torus node.\n");
        #endif

		do_search_torus_node(msg);
		break;

	case UPDATE_SKIP_LIST:
        #ifdef WRITE_LOG
            write_log(TORUS_NODE_LOG, "receive request update skip list.\n");
        #endif

		if (FALSE == do_update_skip_list(msg)) {
			reply_code = FAILED;
		}
		/*reply_msg.op = (OP)msg.op;
		reply_msg.reply_code = (REPLY_CODE)reply_code;
		strncpy(reply_msg.stamp, msg.stamp, STAMP_SIZE);
		if (FALSE == send_reply(socketfd, reply_msg)) {
			// TODO handle send reply failed
			return FALSE;
		}*/
		print_skip_list_node(the_skip_list);
		break;

	case UPDATE_SKIP_LIST_NODE:
        #ifdef WRITE_LOG
            write_log(TORUS_NODE_LOG, "receive request update skip list node.\n");
        #endif

		if (FALSE == do_update_skip_list_node(msg)) {
			reply_code = FAILED;
		}
		reply_msg.op = (OP)msg.op;
		reply_msg.reply_code = (REPLY_CODE)reply_code;
		strncpy(reply_msg.stamp, msg.stamp, STAMP_SIZE);
		if (FALSE == send_reply(socketfd, reply_msg)) {
			// TODO handle send reply failed
			return FALSE;
		}
		print_skip_list_node(the_skip_list);
		break;

	case SEARCH_SKIP_LIST_NODE:
        #ifdef WRITE_LOG
            write_log(TORUS_NODE_LOG, "\nreceive request search skip list node.\n");
        #endif

		if( FALSE == do_search_skip_list_node(msg)) {
            reply_code = FAILED;
        }
		reply_msg.op = (OP)msg.op;
		reply_msg.reply_code = (REPLY_CODE)reply_code;
		strncpy(reply_msg.stamp, msg.stamp, STAMP_SIZE);
		if (FALSE == send_reply(socketfd, reply_msg)) {
			// TODO handle send reply failed
			return FALSE;
		}
		break;

	case UPDATE_FORWARD:
        #ifdef WRITE_LOG
            write_log(TORUS_NODE_LOG, "receive request update skip list node's forward field.\n");
        #endif

		if (FALSE == do_update_forward(msg)) {
			reply_code = FAILED;
		}
		reply_msg.op = (OP)msg.op;
		reply_msg.reply_code = (REPLY_CODE)reply_code;
		strncpy(reply_msg.stamp, msg.stamp, STAMP_SIZE);
		if (FALSE == send_reply(socketfd, reply_msg)) {
			// TODO handle send reply failed
			return FALSE;
		}
		print_skip_list_node(the_skip_list);
		break;

	case UPDATE_BACKWARD:
        #ifdef WRITE_LOG
            write_log(TORUS_NODE_LOG, "receive request update skip list node's backward field.\n");
        #endif

		if (FALSE == do_update_backward(msg)) {
			reply_code = FAILED;
		}
		reply_msg.op = (OP)msg.op;
		reply_msg.reply_code = (REPLY_CODE)reply_code;
		strncpy(reply_msg.stamp, msg.stamp, STAMP_SIZE);
		if (FALSE == send_reply(socketfd, reply_msg)) {
			// TODO handle send reply failed
			return FALSE;
		}
		print_skip_list_node(the_skip_list);
		break;

	case NEW_SKIP_LIST:
        #ifdef WRITE_LOG
            write_log(TORUS_NODE_LOG, "receive request new skip list.\n");
        #endif

		if (FALSE == do_new_skip_list(msg)) {
			reply_code = FAILED;
		}
		reply_msg.op = (OP)msg.op;
		reply_msg.reply_code = (REPLY_CODE)reply_code;
		strncpy(reply_msg.stamp, msg.stamp, STAMP_SIZE);
		if (FALSE == send_reply(socketfd, reply_msg)) {
			// TODO handle send reply failed
			return FALSE;
		}
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
        do_receive_data(socketfd);
        break;
    case PERFORMANCE_TEST:
        if(FALSE == do_performance_test(msg)) {
            reply_code = FAILED;
        }
        reply_msg.op = (OP)msg.op;
        reply_msg.reply_code = (REPLY_CODE)reply_code;
        strncpy(reply_msg.stamp, msg.stamp, STAMP_SIZE);
        if (FALSE == send_reply(socketfd, reply_msg)) {
            // TODO handle send reply failed
            return FALSE;
        }
        break;
    case RELOAD_RTREE:
        #ifdef WRITE_LOG
            write_log(TORUS_NODE_LOG, "receive request reload rtree data.\n");
        #endif
        if(FALSE == do_rtree_load_data(socketfd, msg)) {
            reply_code = FAILED;
        }
        /*reply_msg.op = (OP)msg.op;
        reply_msg.reply_code = (REPLY_CODE)reply_code;
        strncpy(reply_msg.stamp, msg.stamp, STAMP_SIZE);
        if (FALSE == send_reply(socketfd, reply_msg)) {
            // TODO handle send reply failed
            return FALSE;
        }*/
        break;
	default:
		reply_code = (REPLY_CODE)WRONG_OP;
        break;
	}


    // if op is RECEIVE_DATA close the fd by receive data thread
    if(msg.op != RECEIVE_DATA && msg.op != RELOAD_RTREE) {
        close(socketfd);
    }

	return TRUE;
}

int new_server() {
	int server_socket;
	server_socket = new_server_socket();
	if (server_socket == FALSE) {
		return FALSE;
	}
	return server_socket;
}

int main(int argc, char **argv) {

	printf("start torus node.\n");
	write_log(TORUS_NODE_LOG, "start torus node.\n");

	int server_socket, i;
	should_run = 1;

	// new a torus node
	the_torus = NULL; // = new_torus_node();

	the_skip_list = NULL; //*new_skip_list_node();

	// create a new request list
	req_list = new_request();

	// load rtree
    the_torus_rtree = NULL;
	the_torus_rtree = rtree_create();
    if(the_torus_rtree == NULL){
        write_log(TORUS_NODE_LOG, "load rtree failed.\n");
        exit(1);
    }
	printf("load rtree.\n");
	write_log(TORUS_NODE_LOG, "load rtree.\n");


	server_socket = new_server();
	if (server_socket == FALSE) {
        write_log(TORUS_NODE_LOG, "start server failed.\n");
        exit(1);
	}
	printf("start server.\n");
	write_log(TORUS_NODE_LOG, "start server.\n");

    int rcv_buf = 256 * 1024;
    setsockopt(server_socket,SOL_SOCKET,SO_RCVBUF,(const char*)&rcv_buf,sizeof(int)); 

	// set server socket nonblocking
	set_nonblocking(server_socket);

	// epoll
	int epfd;
	struct epoll_event ev, events[MAX_EVENTS];
	epfd = epoll_create(MAX_EVENTS);
	ev.data.fd = server_socket;
	ev.events = EPOLLIN;
	epoll_ctl(epfd, EPOLL_CTL_ADD, server_socket, &ev);

	int nfds;
	while(should_run) {
		nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);
		for(i = 0; i < nfds; i++) {
			int conn_socket;
			if(events[i].data.fd == server_socket) {
				while((conn_socket = accept_connection(server_socket)) > 0) {
					set_nonblocking(conn_socket);

                    ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
					ev.data.fd = conn_socket;
					epoll_ctl(epfd, EPOLL_CTL_ADD, conn_socket, &ev);
				}
			} else {
				conn_socket = events[i].data.fd;
				struct message msg;
				memset(&msg, 0, sizeof(struct message));

				// receive message through the conn_socket
				if (TRUE == receive_message(conn_socket, &msg)) {
					process_message(conn_socket, msg);
				} else {
					//  TODO: handle receive message failed
					printf("receive message failed.\n");
				}
			}
		}
	}

	return 0;
}

