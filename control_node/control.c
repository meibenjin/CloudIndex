/*
 * control.c
 *
 *  Created on: Sep 20, 2013
 *      Author: meibenjin
 */

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/time.h>
#include<time.h>
#include<unistd.h>
#include<errno.h>
#include<assert.h>
#include<pthread.h>

#include"control.h"
#include"torus_node/torus_node.h"
#include"communication/socket.h"
#include"communication/message.h"
#include"communication/message_handler.h"
#include"communication/connection.h"
#include"communication/connection_mgr.h"
#include"skip_list/skip_list.h"
#include"command/command.h"
#include"utils/geometry.h"
#include"logs/log.h"
#include"config/config.h"

#include"data_generator/generator.h"
#include"test/test.h"

__asm__(".symver memcpy,memcpy@GLIBC_2.2.5");

//global configuration
extern configuration_t the_config;

torus_cluster *the_cluster_list = NULL;

// skip list (multiple level linked list for torus cluster)
skip_list *the_skip_list = NULL;

// connections manager 
connection_mgr_t the_conn_mgr = NULL;

extern command_mgr_t the_cmd_mgr;

pthread_mutex_t file_mutex;

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

torus_cluster *new_torus_cluster() {
	torus_cluster *cluster_list;
	cluster_list = (torus_cluster *) malloc(sizeof(torus_cluster));
	if (cluster_list == NULL) {
		return NULL;
	}
	cluster_list->torus = NULL;
	cluster_list->next = NULL;

	return cluster_list;
}

torus_cluster *find_torus_cluster(torus_cluster *list, int cluster_id) {
	torus_cluster *cluster_ptr = list->next;
	while (cluster_ptr) {
		if (cluster_ptr->torus->cluster_id == cluster_id) {
			return cluster_ptr;
		}
		cluster_ptr = cluster_ptr->next;
	}
	return NULL;
}

torus_cluster *insert_torus_cluster(torus_cluster *list, torus_s *torus_ptr) {
	torus_cluster *new_cluster = new_torus_cluster();
	if (new_cluster == NULL) {
		printf("insert_torus_cluster: allocate new torus cluster failed.\n");
		return NULL;
	}
	new_cluster->torus = torus_ptr;

	torus_cluster *cluster_ptr = list;

	while (cluster_ptr->next) {
		cluster_ptr = cluster_ptr->next;
	}

	new_cluster->next = cluster_ptr->next;
	cluster_ptr->next = new_cluster;

	return new_cluster;
}

int remove_torus_cluster(torus_cluster *list, int cluster_id) {
	torus_cluster *pre_ptr = list;
	torus_cluster *cluster_ptr = list->next;
	while (cluster_ptr) {
		if (cluster_ptr->torus->cluster_id == cluster_id) {
			pre_ptr->next = cluster_ptr->next;
			free(cluster_ptr);
			cluster_ptr = NULL;
			return TRUE;
		}
		pre_ptr = pre_ptr->next;
		cluster_ptr = cluster_ptr->next;
	}
	return FALSE;
}

void print_torus_cluster(torus_cluster *list) {
	torus_cluster *cluster_ptr = list->next;
	while (cluster_ptr) {
		printf("cluster:%d\n", cluster_ptr->torus->cluster_id);
		print_torus(cluster_ptr->torus);
		cluster_ptr = cluster_ptr->next;
	}
}

int set_partitions(torus_partitions *torus_p, int p_x, int p_y, int p_z) {
	if ((p_x < 1) || (p_y < 1) || (p_z < 1)) {
		printf("the number of partitions too small.\n");
		return FALSE;
	}
	if ((p_x > 20) || (p_y > 20) || (p_z > 20)) {
		printf("the number of partitions too large.\n");
		return FALSE;
	}

	torus_p->p_x = p_x;
	torus_p->p_y = p_y;
	torus_p->p_z = p_z;

	return TRUE;
}

inline int get_nodes_num(struct torus_partitions t_p) {
	return t_p.p_x * t_p.p_y * t_p.p_z;
}

int translate_coordinates(torus_s *torus, int direction) {
	if (torus == NULL) {
		printf("translate_coordinates: torus is null pointer.\n");
		return FALSE;
	}

	int i, nodes_num;

	torus_partitions t_p = torus->partition;
	nodes_num = get_nodes_num(t_p);

	for (i = 0; i < nodes_num; ++i) {
		struct coordinate c = get_node_id(torus->node_list[i].info);
		switch (direction) {
		case X_L:
		case X_R:
			set_node_id(&torus->node_list[i].info, t_p.p_x + c.x, c.y, c.z);
			break;
		case Y_L:
		case Y_R:
			set_node_id(&torus->node_list[i].info, c.x, t_p.p_y + c.y, c.z);
			break;
		case Z_L:
		case Z_R:
			set_node_id(&torus->node_list[i].info, c.x, c.y, t_p.p_z + c.z);
			break;
		}
	}
	return TRUE;
}

int assign_node_ip(torus_node *node_ptr) {
	//current assigned nodes list index
	static int i = 0;

	if (0 == the_config->nodes_num) {
		write_log(ERROR_LOG, "assign_node_ip: the num of nodes is 0.\n");
		return FALSE;
	}

	if (!node_ptr) {
		printf("assign_node_ip: node_ptr is null pointer.\n");
		return FALSE;
	}

	if (i < the_config->nodes_num) {
		set_node_ip(&node_ptr->info, the_config->nodes[i]);
		i++;
	} else {
		printf("assign_node_ip: no free ip to be assigned.\n");
		return FALSE;
	}

	return TRUE;
}

int assign_cluster_id() {

	static int index = 0;
	//set_cluster_id(&node_ptr->info, index++);
	return index++;
}

int get_node_index(torus_partitions torus_p, int x, int y, int z) {
	return x * torus_p.p_y * torus_p.p_z + y * torus_p.p_z + z;
}

int set_neighbors(torus_s *torus, torus_node *node_ptr) {
	if (!node_ptr) {
		printf("set_neighbors: node_ptr is null pointer.\n");
		return FALSE;
	}
	struct torus_partitions torus_p;
	torus_p = torus->partition;
	struct coordinate c = get_node_id(node_ptr->info);

	// x-coordinate of the right neighbor of node_ptr on x-axis
	int xrc = (c.x + torus_p.p_x + 1) % torus_p.p_x;
	// x-coordinate of the left neighbor of node_ptr on x-axis
	int xlc = (c.x + torus_p.p_x - 1) % torus_p.p_x;

	// y-coordinate of the right neighbor of node_ptr on y-axis
	int yrc = (c.y + torus_p.p_y + 1) % torus_p.p_y;
	// y-coordinate of the left neighbor of node_ptr on y-axis
	int ylc = (c.y + torus_p.p_y - 1) % torus_p.p_y;

	// z-coordinate of the right neighbor of node_ptr on z-axis
	int zrc = (c.z + torus_p.p_z + 1) % torus_p.p_z;
	// z-coordinate of the left neighbor of node_ptr on z-axis
	int zlc = (c.z + torus_p.p_z - 1) % torus_p.p_z;

	// index of the right neighbor of node_ptr on x-axis
	int xri = get_node_index(torus_p, xrc, c.y, c.z);
	// index of the left neighbor of node_ptr on x-axis
	int xli = get_node_index(torus_p, xlc, c.y, c.z);

	// index of the right neighbor of node_ptr on y-axis
	int yri = get_node_index(torus_p, c.x, yrc, c.z);
	// index of the left neighbor of node_ptr on y-axis
	int yli = get_node_index(torus_p, c.x, ylc, c.z);

	// index of the right neighbor of node_ptr on z-axis
	int zri = get_node_index(torus_p, c.x, c.y, zrc);
	// index of the left neighbor of node_ptr on z-axis
	int zli = get_node_index(torus_p, c.x, c.y, zlc);

	int neighbors_num = 0;
	if (xrc != c.x) {
		add_neighbor_info(node_ptr, X_R, &torus->node_list[xri].info);
		neighbors_num++;
	}
	if (xri != xli) {
		add_neighbor_info(node_ptr, X_L, &torus->node_list[xli].info);
		neighbors_num++;
	}

	if (yrc != c.y) {
		add_neighbor_info(node_ptr, Y_R, &torus->node_list[yri].info);
		neighbors_num++;
	}
	if (yri != yli) {
		add_neighbor_info(node_ptr, Y_L, &torus->node_list[yli].info);
		neighbors_num++;
	}

	if (zrc != c.z) {
		add_neighbor_info(node_ptr, Z_R, &torus->node_list[zri].info);
		neighbors_num++;
	}
	if (zri != zli) {
		add_neighbor_info(node_ptr, Z_L, &torus->node_list[zli].info);
		neighbors_num++;
	}

	set_neighbors_num(node_ptr, neighbors_num);

	return TRUE;
}

// TODO choose LEADER_NUM leder nodes
// this would be done after torus interval has been assigned;
void assign_torus_leader(torus_s *torus) {
	int i, j;
	int nodes_num = get_nodes_num(torus->partition);

	struct interval total_region[MAX_DIM_NUM];
	for (i = 0; i < MAX_DIM_NUM; ++i) {
		total_region[i] = torus->node_list[0].info.region[i];
	}

	//calculate total interval in a torus cluster
	for (i = 0; i < MAX_DIM_NUM; ++i) {
		for (j = 1; j < nodes_num; ++j) {
			if (torus->node_list[j].info.region[i].low < total_region[i].low) {
				total_region[i].low = torus->node_list[j].info.region[i].low;
			}
			if (torus->node_list[j].info.region[i].high
					> total_region[i].high) {
				total_region[i].high = torus->node_list[j].info.region[i].high;
			}
		}
	}

	int num = nodes_num > LEADER_NUM ? nodes_num : LEADER_NUM;

	int index[num];
	// this function rearrange a initial array indexed with 0~n-1
	shuffle(index, num);

	// random choose LEADER_NUM torus nodes as leader
	int k;
	for (i = 0; i < LEADER_NUM; ++i) {
		// in case the index is larger than the nodes_num
		if (index[i] > nodes_num - 1) {
			k = nodes_num - 1;
		} else {
			k = index[i];
		}
		torus->leaders[i] = torus->node_list[k].info;
		for (j = 0; j < MAX_DIM_NUM; ++j) {
			torus->leaders[i].region[j] = total_region[j];
		}
		// update leader flag
		torus->node_list[k].is_leader = 1;
	}
}

void shuffle(int array[], int n) {
	int i, tmp;

	for (i = 0; i < n; i++) {
		array[i] = i;
	}

	// TODO: uncomment it after all torus node has the same configuration
	for (i = n; i > 0; --i) {
		int j = rand() % i;

		//swap
		tmp = array[i - 1];
		array[i - 1] = array[j];
		array[j] = tmp;
	}
}

torus_s *new_torus(struct torus_partitions new_torus_p) {
	int i, j, k, index, nodes_num;

	nodes_num = get_nodes_num(new_torus_p);
	if (nodes_num <= 0) {
		printf("new_torus: incorrect partition.\n");
		return NULL;
	}

	struct torus_s *torus_ptr;

	torus_ptr = (torus_s *) malloc(sizeof(torus_s));
	if (torus_ptr == NULL) {
		printf("new_torus: malloc space for new torus failed.\n");
		return NULL;
	}

	// assign new torus cluster id
	torus_ptr->cluster_id = assign_cluster_id();

	// set new torus's partition info
	torus_ptr->partition = new_torus_p;

	torus_ptr->node_list = (torus_node *) malloc(
			sizeof(torus_node) * nodes_num);

	if (torus_ptr->node_list == NULL) {
		printf("new_torus: malloc space for torus node list failed.\n");
		return NULL;
	}

	index = 0;
	struct torus_node *new_node;
	for (i = 0; i < new_torus_p.p_x; ++i) {
		for (j = 0; j < new_torus_p.p_y; ++j) {
			for (k = 0; k < new_torus_p.p_z; ++k) {
				new_node = &torus_ptr->node_list[index];

				init_torus_node(new_node);
				if (assign_node_ip(new_node) == FALSE) {
					free(torus_ptr->node_list);
					return NULL;
				}

				set_cluster_id(&new_node->info, torus_ptr->cluster_id);
				set_node_id(&new_node->info, i, j, k);
				set_node_capacity(&new_node->info, DEFAULT_CAPACITY);
				index++;
			}
		}
	}

	return torus_ptr;
}

torus_s *create_torus(torus_partitions tp) {
	int i, j, k, index, nodes_num;

	torus_partitions new_torus_p;
	if (FALSE == set_partitions(&new_torus_p, tp.p_x, tp.p_y, tp.p_z)) {
		return NULL;
	}

	nodes_num = get_nodes_num(new_torus_p);
	if (nodes_num <= 0) {
		printf("create_torus: incorrect partition.\n");
		return NULL;
	}

	// create a new torus
	torus_s *torus_ptr;
	torus_ptr = new_torus(new_torus_p);
	if (torus_ptr == NULL) {
		printf("create_torus: create a new torus failed.\n");
		return NULL;
	}

	// set data region for each torus node
	index = 0;
	struct torus_node *pnode;
	for (i = 0; i < new_torus_p.p_x; ++i) {
		for (j = 0; j < new_torus_p.p_y; ++j) {
			for (k = 0; k < new_torus_p.p_z; ++k) {
				pnode = &torus_ptr->node_list[index];
				if (FALSE
						== set_interval(&pnode->info, new_torus_p,
								the_config->data_region)) {
					return NULL;
				}
				index++;
			}
		}
	}

	// set neighbors for each torus node
	for (i = 0; i < nodes_num; ++i) {
		set_neighbors(torus_ptr, &torus_ptr->node_list[i]);
	}

	// assign torus leader for new create torus
	assign_torus_leader(torus_ptr);

	return torus_ptr;
}

torus_s *append_torus(torus_s *to, torus_s *from, int direction) {

	if (to == NULL || from == NULL) {
		printf("append_torus: torus to be appended is null.\n");
		return to;
	}

	// change torus from's coordinates
	switch (direction) {
	case X_R:
	case Y_R:
	case Z_R:
		translate_coordinates(from, direction);
		break;
	case X_L:
	case Y_L:
	case Z_L:
		translate_coordinates(to, direction);
		break;
	}

	int i, index, to_num, from_num, merged_num;
	struct coordinate c;
	struct torus_s *merged_torus;
	struct torus_partitions merged_partition;

	// calc merged torus partitions info
	merged_partition = to->partition;
	switch (direction) {
	case X_L:
	case X_R:
		merged_partition.p_x += from->partition.p_x;
		break;
	case Y_L:
	case Y_R:
		merged_partition.p_y += from->partition.p_y;
		break;
	case Z_L:
	case Z_R:
		merged_partition.p_z += from->partition.p_z;
		break;
	}

	merged_torus = new_torus(merged_partition);
	if (merged_torus == NULL) {
		printf("append_torus: create a new torus failed.\n");
		return to;
	}

	to_num = get_nodes_num(to->partition);
	from_num = get_nodes_num(from->partition);

	for (i = 0; i < to_num; ++i) {
		c = get_node_id(to->node_list[i].info);
		index = get_node_index(to->partition, c.x, c.y, c.z);
		merged_torus->node_list[index] = to->node_list[i];
	}

	for (i = 0; i < from_num; ++i) {
		c = get_node_id(from->node_list[i].info);
		index = get_node_index(from->partition, c.x, c.y, c.z);
		merged_torus->node_list[index] = from->node_list[i];
	}

	merged_num = get_nodes_num(merged_partition);
	for (i = 0; i < merged_num; ++i) {
		set_neighbors(merged_torus, &merged_torus->node_list[i]);
	}

	// free torus from and to
	free(to->node_list);
	free(from->node_list);

	return merged_torus;
}

int dispatch_torus(torus_s *torus) {
	int i, j, nodes_num;

	nodes_num = get_nodes_num(torus->partition);
	// send info to all torus nodes
	for (i = 0; i < nodes_num; ++i) {
		int d;
		size_t cpy_len = 0;
		char buf[DATA_SIZE];
		memset(buf, 0, DATA_SIZE);

		//copy torus partitions into buf
		memcpy(buf, (void *) &torus->partition,
				sizeof(struct torus_partitions));
		cpy_len += sizeof(struct torus_partitions);

		// copy leaders info into buf
		int leaders_num = LEADER_NUM;
		memcpy(buf + cpy_len, &leaders_num, sizeof(int));
		cpy_len += sizeof(int);

		for (j = 0; j < LEADER_NUM; j++) {
			memcpy(buf + cpy_len, &torus->leaders[j], sizeof(node_info));
			cpy_len += sizeof(node_info);
		}

		struct torus_node *node_ptr;
		node_ptr = &torus->node_list[i];

		//copy current torus node's leader flag into buf
		memcpy(buf + cpy_len, &node_ptr->is_leader, sizeof(int));
		cpy_len += sizeof(int);

		//copy current torus node info into buf
		memcpy(buf + cpy_len, (void *) &node_ptr->info, sizeof(node_info));
		cpy_len += sizeof(node_info);

		// copy current torus node's neighbors info into buf
		for (d = 0; d < DIRECTIONS; d++) {
			int num = get_neighbors_num_d(node_ptr, d);
			memcpy(buf + cpy_len, &num, sizeof(int));
			cpy_len += sizeof(int);

			if (node_ptr->neighbors[d] != NULL) {
				struct neighbor_node *nn_ptr;
				nn_ptr = node_ptr->neighbors[d]->next;
				while (nn_ptr != NULL) {
					memcpy(buf + cpy_len, nn_ptr->info, sizeof(node_info));
					cpy_len += sizeof(node_info);
					nn_ptr = nn_ptr->next;
				}
			}
		}

		char dst_ip[IP_ADDR_LENGTH];
		memset(dst_ip, 0, IP_ADDR_LENGTH);
		get_node_ip(node_ptr->info, dst_ip);

		// get local ip address
		struct message msg;
		char local_ip[IP_ADDR_LENGTH];
		memset(local_ip, 0, IP_ADDR_LENGTH);
		if (FALSE == get_local_ip(local_ip)) {
			return FALSE;
		}
		msg.msg_size = calc_msg_header_size() + cpy_len;
		fill_message(msg.msg_size, CREATE_TORUS, local_ip, dst_ip, "", buf,
				cpy_len, &msg);

		/*if(TRUE == send_message(msg)) {
            printf("send basic info to %s ... succeed.\n", dst_ip);
        } else {
            printf("send basic info to %s ... failed.\n", dst_ip);
        }*/
		if(FALSE == send_message(msg)) {
            return FALSE;
        }


	}
	return TRUE;
}

int traverse_torus(const char *entry_ip) {
	int socketfd;

	socketfd = new_client_socket(entry_ip, MANUAL_WORKER_PORT);
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
	msg.msg_size = calc_msg_header_size() + 1;
	fill_message(msg.msg_size, TRAVERSE_TORUS, local_ip, entry_ip, "", "", 1,
			&msg);
	send_data(socketfd, (void *) &msg, msg.msg_size);
	close(socketfd);

	return TRUE;
}

void print_torus(torus_s *torus) {
	int i, nodes_num;
	nodes_num = get_nodes_num(torus->partition);

	for (i = 0; i < nodes_num; ++i) {
		print_torus_node(torus->node_list[i]);
	}
}

int traverse_skip_list(const char *entry_ip) {
	int socketfd;

	socketfd = new_client_socket(entry_ip, MANUAL_WORKER_PORT);
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
	msg.msg_size = calc_msg_header_size() + 1;
	fill_message(msg.msg_size, TRAVERSE_SKIP_LIST, local_ip, entry_ip, "", "",
			1, &msg);
	send_data(socketfd, (void *) &msg, msg.msg_size);
	close(socketfd);
	return TRUE;
}

int dispatch_skip_list(skip_list *list, node_info leaders[]) {
	int i, j, new_level;
	size_t cpy_len = 0;
	char src_ip[IP_ADDR_LENGTH], dst_ip[IP_ADDR_LENGTH];
	skip_list_node *sln_ptr, *new_sln;
	node_info forwards[LEADER_NUM], backwards[LEADER_NUM], end_node;
	init_node_info(&end_node);

	memset(src_ip, 0, IP_ADDR_LENGTH);
	if (FALSE == get_local_ip(src_ip)) {
		return FALSE;
	}

	// random choose a level and create new skip list node (local create)
	new_level = random_level();
	new_sln = new_skip_list_node(new_level, leaders);

	// create a new skip list node at each leader node (remote create)
	struct message msg;
	msg.op = NEW_SKIP_LIST;
	strncpy(msg.src_ip, src_ip, IP_ADDR_LENGTH);
	strncpy(msg.stamp, "", STAMP_SIZE);

	for (i = 0; i < LEADER_NUM; ++i) {
		cpy_len = 0;
		strncpy(msg.dst_ip, leaders[i].ip, IP_ADDR_LENGTH);
		memset(msg.data, 0, DATA_SIZE);
		memcpy(msg.data, &new_level, sizeof(int));
		cpy_len += sizeof(int);
		memcpy(msg.data + cpy_len, leaders, sizeof(node_info) * LEADER_NUM);
		cpy_len += sizeof(node_info) * LEADER_NUM;

		msg.msg_size = calc_msg_header_size() + cpy_len;

        if(FALSE == send_message(msg)) {
            return FALSE;
        }
	}

	// update skip list header's level
	if (new_level > list->level) {
		list->level = new_level;
	}

	sln_ptr = list->header;
	for (i = list->level; i >= 0; --i) {
		while ((sln_ptr->level[i].forward != NULL)
				&& (-1
						== compare(
								sln_ptr->level[i].forward->leader[0].region[2],
								leaders[0].region[2]))) {
			sln_ptr = sln_ptr->level[i].forward;
		}

		if (i <= new_level) {
			int update_forward = 0, update_backword = 0;
			// update new skip list node's forward field (local update)
			new_sln->level[i].forward = sln_ptr->level[i].forward;
			for (j = 0; j < LEADER_NUM; j++) {
				if (sln_ptr->level[i].forward != NULL) {
					forwards[j] = sln_ptr->level[i].forward->leader[j];
					update_forward = 1;
				}
			}

			// update new skip list node's backward field (local update)
			new_sln->level[i].backward =
					(sln_ptr == list->header) ? NULL : sln_ptr;
			for (j = 0; j < LEADER_NUM; j++) {
				if (sln_ptr != list->header) {
					backwards[j] = sln_ptr->leader[j];
					update_backword = 1;
				}
			}

			//update new skip list node's forwards and backwards (remote update)
			for (j = 0; j < LEADER_NUM; ++j) {
				msg.op = UPDATE_SKIP_LIST_NODE;
				get_node_ip(leaders[j], dst_ip);
				strncpy(msg.dst_ip, dst_ip, IP_ADDR_LENGTH);
				cpy_len = 0;
				memcpy(msg.data, &i, sizeof(int));
				cpy_len += sizeof(int);
				memcpy(msg.data + cpy_len, &update_forward, sizeof(int));
				cpy_len += sizeof(int);
				memcpy(msg.data + cpy_len, &update_backword, sizeof(int));
				cpy_len += sizeof(int);

				if (update_forward == 1) {
					memcpy(msg.data + cpy_len, forwards,
							sizeof(node_info) * LEADER_NUM);
					cpy_len += sizeof(node_info) * LEADER_NUM;
				}

				if (update_backword == 1) {
					memcpy(msg.data + cpy_len, backwards,
							sizeof(node_info) * LEADER_NUM);
					cpy_len += sizeof(node_info) * LEADER_NUM;
				}

				msg.msg_size = calc_msg_header_size() + cpy_len;

                if(FALSE == send_message(msg)) {
                    return FALSE;
                }

			}

			// if current skip list node's forward field exist(not the tail node)
			// update it's(sln_ptr->level[i].forward) backward field (local update)
			if (sln_ptr->level[i].forward) {
				sln_ptr->level[i].forward->level[i].backward = new_sln;

				//update current skip list node's forward's backward (remote update)
				for (j = 0; j < LEADER_NUM; ++j) {
					msg.op = UPDATE_SKIP_LIST_NODE;
					get_node_ip(sln_ptr->level[i].forward->leader[j], dst_ip);
					strncpy(msg.dst_ip, dst_ip, IP_ADDR_LENGTH);
					cpy_len = 0;

					// only need update backward
					update_forward = 0;
					update_backword = 1;
					memcpy(msg.data, &i, sizeof(int));
					cpy_len += sizeof(int);
					memcpy(msg.data + cpy_len, &update_forward, sizeof(int));
					cpy_len += sizeof(int);
					memcpy(msg.data + cpy_len, &update_backword, sizeof(int));
					cpy_len += sizeof(int);
					memcpy(msg.data + cpy_len, leaders,
							sizeof(node_info) * LEADER_NUM);
					cpy_len += sizeof(node_info) * LEADER_NUM;

					msg.msg_size = calc_msg_header_size() + cpy_len;

                    if(FALSE == send_message(msg)) {
                        return FALSE;
                    }
				}
			}

			// update current skip list node's forward field (local update)
			sln_ptr->level[i].forward = new_sln;
			for (j = 0; j < LEADER_NUM; ++j) {

				//update current skip list node's forward (remote update)
				if (get_cluster_id(sln_ptr->leader[j]) != -1) {
					msg.op = UPDATE_SKIP_LIST_NODE;
					get_node_ip(sln_ptr->leader[j], dst_ip);
					strncpy(msg.dst_ip, dst_ip, IP_ADDR_LENGTH);
					cpy_len = 0;
					// only need update forward
					update_forward = 1;
					update_backword = 0;
					memcpy(msg.data, &i, sizeof(int));
					cpy_len += sizeof(int);
					memcpy(msg.data + cpy_len, &update_forward, sizeof(int));
					cpy_len += sizeof(int);
					memcpy(msg.data + cpy_len, &update_backword, sizeof(int));
					cpy_len += sizeof(int);
					memcpy(msg.data + cpy_len, leaders,
							sizeof(node_info) * LEADER_NUM);
					cpy_len += sizeof(node_info) * LEADER_NUM;

					msg.msg_size = calc_msg_header_size() + cpy_len;

                    if(FALSE == send_message(msg)) {
                        return FALSE;
                    }
				}
			}
		}
	}

	return TRUE;
}

int new_torus_cluster_mgr() {
	// create a new cluster list
	the_cluster_list = new_torus_cluster();
	if (NULL == the_cluster_list) {
		return FALSE;
	}

	// create a new skip list
	the_skip_list = new_skip_list(MAXLEVEL);
	if (NULL == the_skip_list) {
		return FALSE;
	}
	return TRUE;
}


int cmd_create_torus_cluster(void *args) {
    // if the_cluster_list is null
    // the_skip_list must be null
    if(the_cluster_list == NULL) {
        // create a new cluster list
        the_cluster_list = new_torus_cluster();
        if (NULL == the_cluster_list) {
            return FALSE;
        }

        // create a new skip list
        the_skip_list = new_skip_list(MAXLEVEL);
        if (NULL == the_skip_list) {
            return FALSE;
        }
    } else {
        printf("the tours cluster has been created.\n");
        return FALSE;
    }

    // create each cluster based on partitions config
	int cnt = 0;
	while (cnt < the_config->cluster_num) {
		struct torus_s *torus_ptr;
		torus_ptr = create_torus(the_config->partitions[cnt]);
		if (torus_ptr == NULL) {
			return FALSE;
		}
		// write current torus leader ip into file
		write_torus_leaders(torus_ptr->leaders);

		// insert newly created torus cluster into the_cluster_list
		insert_torus_cluster(the_cluster_list, torus_ptr);
		cnt++;
    }

    printf("create torus cluster ... succeed.\n");
    //print_torus_cluster(the_cluster_list);

	return TRUE;
}

// process "initcluster" command
int cmd_deploy_torus_cluster(void *args) {

    if(the_cluster_list == NULL) {
        printf("the torus cluster doesn't create.\n");
        return FALSE;
    }

	torus_cluster *ptr = the_cluster_list->next;
	while (ptr) {
		struct torus_s *torus_ptr = ptr->torus;
        // send initialize info to torus nodes;
        if(FALSE == dispatch_torus(torus_ptr)) {
            printf("send initialize info to torus node ... failed.\n");
            return FALSE;
        }

        // send route info to torus nodes
        if(FALSE == dispatch_skip_list(the_skip_list, torus_ptr->leaders)) {
            printf("send route info to torus node ... failed.\n");
            return FALSE;
        }

		ptr = ptr->next;
	}
    printf("send cluster info ... succeed.\n");
    printf("input 'more' command for more information.\n");
    return TRUE;
}

int cmd_help(void *args) {
    printf("support commands:\n\n");
    print_command_list(the_cmd_mgr);
    return TRUE;
}


int cmd_exit(void *args) {
    stop_connection_mgr(the_conn_mgr);
    stop_command_mgr();
    printf("bye.\n");
    exit(0);
    return TRUE;
}

int cmd_more_response(void *args) {
    command *cmd = &the_cmd_mgr->last_cmd;

    pthread_mutex_lock(&file_mutex);

    FILE *fp = fopen(cmd->cache_file, "r");
    if(NULL == fp) {
        printf("no response.\n");
        pthread_mutex_unlock(&file_mutex);
        return FALSE;
    }
    printf("command %s response:\n", cmd->name);

    char read_buf[BUF_SIZE + 1];
    memset(read_buf, 0, BUF_SIZE + 1);
    while (!feof(fp)) {
        fread(read_buf, BUF_SIZE, 1, fp);
        printf("%s", read_buf);
        memset(read_buf, 0, BUF_SIZE + 1);
    }
    fclose(fp);

    pthread_mutex_unlock(&file_mutex);

    return TRUE;
}

int cmd_cluster_status(void *args) {
    check_system_status(0);
    printf("check cluster statuc ... succeed.\n");
    printf("input 'more' command for more information.\n");
    return TRUE;
}

int cmd_insert_data(void *args) {
    // invoke data insert function at data_generator
    insert_data(NULL, 50000);

    printf("insert data to cluster ... succeed.\n");
    printf("input 'clusterstatus' command to get more information.\n");
    return TRUE;
}

int cmd_range_query(void *args) {
    query("172.16.0.179", "range_query", 500);
    sleep(1);
    printf("exec range query ... succeed.\n");
    printf("input 'more' command for more information.\n");
    return TRUE;
}

int cmd_nn_query(void *args) {
    query("172.16.0.179", "nn_query", 500);
    sleep(1);
    printf("exec nn query ... succeed.\n");
    printf("input 'more' command for more information.\n");
    return TRUE;
}

int do_write_reply(message msg) {
    command *cmd = &the_cmd_mgr->last_cmd;

    pthread_mutex_lock(&file_mutex);
    write_log(cmd->cache_file, "%s", msg.data);
    pthread_mutex_unlock(&file_mutex);

    return TRUE;
}

static int register_message_handler_list() {
    // TODO register all message
    register_message_handler2(REP_CREATE_TORUS, &do_write_reply);
    register_message_handler2(REP_NEW_SKIP_LIST, &do_write_reply);
    register_message_handler2(REP_INSERT_DATA, &do_write_reply);
    register_message_handler2(REP_CHECK_SYSTEM_STATUS, &do_write_reply);
    register_message_handler2(REP_RANGE_QUERY, &do_write_reply);
    register_message_handler2(REP_NN_QUERY, &do_write_reply);

    return TRUE;
}

static int load_command_list() {
    add_command("newcluster", "create a new torus cluster (local)", LOCAL_CMD,  &cmd_create_torus_cluster);
	add_command("initcluster", "send initial node info and route info to torus cluster (remote)", REMOTE_CMD, &cmd_deploy_torus_cluster);
	add_command("insertdata", "insert spatio-temporal data into torus cluster (remote)", REMOTE_CMD, &cmd_insert_data);
	add_command("clusterstatus", "return torus cluster status (remote)", REMOTE_CMD, &cmd_cluster_status);
	add_command("rangequery", "range query [file path] (remote)", REMOTE_CMD, &cmd_range_query);
	add_command("nnquery", "nn query [file path] (remote)", REMOTE_CMD, &cmd_nn_query);
	add_command("more", "print previous remote command's response from torus cluster", LOCAL_CMD, &cmd_more_response);
	add_command("help", "print supported commands (local)", LOCAL_CMD, &cmd_help);
	add_command("quit", "quit command line (local)", LOCAL_CMD, &cmd_exit);

    return TRUE;
}


int main(int argc, char **argv) {

	// create a new configuration
	the_config = new_configuration();
	if (NULL == the_config) {
		exit(1);
	}
	// load all configuration from file
	if (FALSE == load_configuration(the_config)) {
		exit(1);
	}
	update_properties(the_config->props);

	// create a torus cluster manager instance
	/*if (FALSE == new_torus_cluster_mgr()) {
		exit(1);
	}*/

	the_msg_mgr = new_message_mgr();
	if (NULL == the_msg_mgr) {
		exit(1);
	}

    // load message handler collection
    register_message_handler_list();

	the_cmd_mgr = new_command_mgr();
	if (NULL == the_cmd_mgr) {
		exit(1);
	}
	// load all commands
	load_command_list();


    // mutex for file
    pthread_mutex_init(&file_mutex, NULL);

	// run a command mgr thread and begin to accept command
	pthread_t cmd_thread;
	cmd_thread = run_command_mgr(the_cmd_mgr);

    // init current torus node's connection manager
    the_conn_mgr = new_connection_mgr();
    if(NULL == the_conn_mgr) {
        write_log(ERROR_LOG, "new connection manager failed.\n");
    }

    init_connection_mgr(the_conn_mgr, &handle_read_event, &handle_write_event);
    run_connection_mgr(the_conn_mgr);

	/* wait for command line thread finish
	 * */
	pthread_join(cmd_thread, NULL);
	//deploy_torus_cluster();
    printf("bye.\n");
	return 0;
}

