/*
 * control.c
 *
 *  Created on: Sep 20, 2013
 *      Author: meibenjin
 */

#include"control.h"

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>
#include<unistd.h>

__asm__(".symver memcpy,memcpy@GLIBC_2.2.5");

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

int get_nodes_num(struct torus_partitions t_p) {
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
		case X_R:
		case X_L:
			set_node_id(&torus->node_list[i].info, t_p.p_x + c.x, c.y, c.z);
			break;
		case Y_R:
		case Y_L:
			set_node_id(&torus->node_list[i].info, c.x, t_p.p_y + c.y, c.z);
			break;
		case Z_R:
		case Z_L:
			set_node_id(&torus->node_list[i].info, c.x, c.y, t_p.p_z + c.z);
			break;
		}
	}
	return TRUE;
}

int assign_node_ip(torus_node *node_ptr) {
	static int i = 0;
	if (torus_ip_list == NULL) {
		printf("assign_node_ip: torus_ip_list is null pointer.\n");
		return FALSE;
	}

	if (!node_ptr) {
		printf("assign_node_ip: node_ptr is null pointer.\n");
		return FALSE;
	}
	if (i < torus_nodes_num) {
		set_node_ip(&node_ptr->info, torus_ip_list[i++]);
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

	int i = 0;
	if (xrc != c.x) {
		node_ptr->neighbors[i++] = &torus->node_list[xri];
	}
	if (xri != xli) {
		node_ptr->neighbors[i++] = &torus->node_list[xli];
	}

	if (yrc != c.y) {
		node_ptr->neighbors[i++] = &torus->node_list[yri];
	}
	if (yri != yli) {
		node_ptr->neighbors[i++] = &torus->node_list[yli];
	}

	if (zrc != c.z) {
		node_ptr->neighbors[i++] = &torus->node_list[zri];
	}
	if (zri != zli) {
		node_ptr->neighbors[i++] = &torus->node_list[zli];
	}

	set_neighbors_num(node_ptr, i);

	return TRUE;
}

//TODO choose a leader node
node_info *assign_torus_leader(torus_s *torus) {
	node_info *leader_node = (node_info*) malloc(sizeof(node_info));
	if (leader_node == NULL) {
		return NULL;
	}

	int i, j;
	int nodes_num = get_nodes_num(torus->partition);

	// default set 0# torus node as leader node
	*leader_node = torus->node_list[0].info;

	//calculate total interval in a torus cluster
	for (i = 0; i < MAX_DIM_NUM; ++i) {
		for (j = 1; j < nodes_num; ++j) {
			if (torus->node_list[j].info.dims[i].low
					< leader_node->dims[i].low) {
				leader_node->dims[i].low = torus->node_list[j].info.dims[i].low;
			}
			if (torus->node_list[j].info.dims[i].high
					> leader_node->dims[i].high) {
				leader_node->dims[i].high =
						torus->node_list[j].info.dims[i].high;
			}
		}
	}
	return leader_node;
}

torus_s *new_torus(struct torus_partitions new_torus_p) {
	int nodes_num;
	nodes_num = get_nodes_num(new_torus_p);
	if (nodes_num <= 0) {
		printf("new_torus: incorrect partition.\n");
		return NULL;
	}

	struct torus_s *torus_ptr;
	struct torus_node *new_node;

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

	int i, j, k, index;

	index = 0;
	//static int last_dim[MAX_DIM_NUM] = { 1, 1, 1 };
	interval intvl[MAX_DIM_NUM];
	int d_x[new_torus_p.p_x + 1];
	int d_y[new_torus_p.p_y + 1];
	int d_z[new_torus_p.p_z + 1];

	int range = 100;

	for (i = 1, d_x[0] = 0; i <= new_torus_p.p_x; ++i) {
		if (i == new_torus_p.p_x) {
			d_x[i] = 100;
			break;
		}
		d_x[i] = d_x[i - 1] + range / new_torus_p.p_x;
	}
	for (j = 1, d_y[0] = 0; j <= new_torus_p.p_y; ++j) {
		if (j == new_torus_p.p_y) {
			d_y[j] = 100;
			break;
		}
		d_y[j] = d_y[j - 1] + range / new_torus_p.p_y;
	}
	for (k = 1, d_z[0] = 0; k <= new_torus_p.p_z; ++k) {
		if (k == new_torus_p.p_z) {
			d_z[k] = 100;
			break;
		}
		d_z[k] = d_z[k - 1] + range / new_torus_p.p_z;
	}

	for (i = 0; i < new_torus_p.p_x; ++i) {
		intvl[0].low = d_x[i] + 1;
		intvl[0].high = d_x[i + 1];
		for (j = 0; j < new_torus_p.p_y; ++j) {
			intvl[1].low = d_y[j] + 1;
			intvl[1].high = d_y[j + 1];
			for (k = 0; k < new_torus_p.p_z; ++k) {
				intvl[2].low = d_z[k] + 1;
				intvl[2].high = d_z[k + 1];

				new_node = &torus_ptr->node_list[index];

				init_torus_node(new_node);
				if (assign_node_ip(new_node) == FALSE) {
					free(torus_ptr);
					return NULL;
				}

				int t;
				for (t = 0; t < MAX_DIM_NUM; ++t) {
					new_node->info.dims[t].low = intvl[t].low;
					new_node->info.dims[t].high = intvl[t].high;
				}

				//assign_dimensions(&new_node->info);

				set_cluster_id(&new_node->info, torus_ptr->cluster_id);
				set_node_id(&new_node->info, i, j, k);
				index++;
			}
		}
	}

	return torus_ptr;
}

torus_s *create_torus(int p_x, int p_y, int p_z) {
	int i, nodes_num;

	torus_partitions new_torus_p;
	if (FALSE == set_partitions(&new_torus_p, p_x, p_y, p_z)) {
		return NULL;
	}

	nodes_num = get_nodes_num(new_torus_p);
	if (nodes_num <= 0) {
		printf("create_torus: incorrect partition.\n");
		return NULL;
	}

	torus_s *torus_ptr;
	// create a new torus
	torus_ptr = new_torus(new_torus_p);
	if (torus_ptr == NULL) {
		printf("create_torus: create a new torus failed.\n");
		return NULL;
	}

	for (i = 0; i < nodes_num; ++i) {
		set_neighbors(torus_ptr, &torus_ptr->node_list[i]);
	}

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
	case X_R:
	case X_L:
		merged_partition.p_x += from->partition.p_x;
		break;
	case Y_R:
	case Y_L:
		merged_partition.p_y += from->partition.p_y;
		break;
	case Z_R:
	case Z_L:
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
	free(to);
	free(from);

	return merged_torus;
}

int send_partition_info(const char *dst_ip, struct torus_partitions torus_p) {
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
	msg.op = UPDATE_PARTITION;
	strncpy(msg.src_ip, local_ip, IP_ADDR_LENGTH);
	strncpy(msg.dst_ip, dst_ip, IP_ADDR_LENGTH);
	strncpy(msg.stamp, "", STAMP_SIZE);
	memcpy(msg.data, &torus_p, sizeof(struct torus_partitions));

	int ret;
	struct reply_message reply_msg;
	if (TRUE == send_message(socketfd, msg)) {
		if (TRUE == receive_reply(socketfd, &reply_msg)) {
			if (SUCCESS == reply_msg.reply_code) {
				printf("%s: send partition info ...... finish.\n", dst_ip);
				ret = TRUE;
			} else {
				printf("%s: send partition info ...... error occurred.\n",
						dst_ip);
				ret = FALSE;
			}
		} else {
			printf("%s: receive reply ...... failed.\n", dst_ip);
			ret = FALSE;
		}
	} else {
		printf("%s: send partition info...... failed.\n", dst_ip);
		ret = FALSE;
	}
	close(socketfd);
	return ret;

}

int send_nodes_info(OP op, const char *dst_ip, int nodes_num,
		struct node_info *nodes) {
	int i;
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
	msg.op = op;
	strncpy(msg.src_ip, local_ip, IP_ADDR_LENGTH);
	strncpy(msg.dst_ip, dst_ip, IP_ADDR_LENGTH);
	strncpy(msg.stamp, "", STAMP_SIZE);
	memcpy(msg.data, &nodes_num, sizeof(int));
	for (i = 0; i < nodes_num; i++) {
		memcpy(msg.data + sizeof(int) + sizeof(struct node_info) * i,
				(void *) &nodes[i], sizeof(struct node_info));
	}

	int ret;
	struct reply_message reply_msg;
	if (TRUE == send_message(socketfd, msg)) {
		if (TRUE == receive_reply(socketfd, &reply_msg)) {
			if (SUCCESS == reply_msg.reply_code) {
				printf("%s: send nodes info ...... finish.\n", dst_ip);
				ret = TRUE;
			} else {
				printf("%s: send nodes info ...... error occurred.\n", dst_ip);
				ret = FALSE;
			}
		} else {
			printf("%s: receive reply ...... failed.\n", dst_ip);
			ret = FALSE;
		}
	} else {
		printf("%s: send nodes info...... failed.\n", dst_ip);
		ret = FALSE;
	}

	close(socketfd);
	return ret;
}

int update_torus(torus_s *torus) {
	int i, j, index, nodes_num;

	nodes_num = get_nodes_num(torus->partition);
	for (i = 0; i < nodes_num; ++i) {
		int neighbors_num = get_neighbors_num(torus->node_list[i]);

		// the nodes array includes dst nodes and its' neighbors
		node_info nodes[neighbors_num + 1];

		// add dst node itself
		nodes[0] = torus->node_list[i].info;

		struct torus_node *node_ptr = NULL;
		node_ptr = &torus->node_list[i];
		for (j = 0; j < neighbors_num; ++j) {
			if (node_ptr && node_ptr->neighbors[j]) {
				nodes[j + 1] = node_ptr->neighbors[j]->info;
				index++;
			}
		}

		char dst_ip[IP_ADDR_LENGTH];
		memset(dst_ip, 0, IP_ADDR_LENGTH);
		get_node_ip(node_ptr->info, dst_ip);

		// send to dst_ip
		if (FALSE
				== send_nodes_info(UPDATE_TORUS, dst_ip, neighbors_num + 1,
						nodes)) {
			return FALSE;
		}

		send_partition_info(dst_ip, torus->partition);

	}
	return TRUE;
}

int traverse_torus(const char *entry_ip) {
	int socketfd;

	socketfd = new_client_socket(entry_ip);
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
	msg.op = TRAVERSE_TORUS;
	strncpy(msg.src_ip, local_ip, IP_ADDR_LENGTH);
	strncpy(msg.dst_ip, entry_ip, IP_ADDR_LENGTH);
	memset(msg.stamp, 0, STAMP_SIZE);
	memset(msg.data, 0, DATA_SIZE);

	send_message(socketfd, msg);
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

int read_torus_ip_list() {
	FILE *fp;
	char ip[IP_ADDR_LENGTH];
	int count;
	/*torus_ip_list = (char (*)[IP_ADDR_LENGTH]) malloc(
	 sizeof(char *) * MAX_NODES_NUM);*/
	/*if (torus_ip_list == NULL) {
	 printf("read_torus_ip_list: allocate torus_ip_list failed.\n");
	 return FALSE;
	 }*/
	fp = fopen(TORUS_IP_LIST, "rb");
	if (fp == NULL) {
		printf("read_torus_ip_list: open file %s failed.\n", TORUS_IP_LIST);
		return FALSE;
	}

	count = 0;
	while ((fgets(ip, IP_ADDR_LENGTH, fp)) != NULL) {
		ip[strlen(ip) - 1] = '\0';
		strncpy(torus_ip_list[count++], ip, IP_ADDR_LENGTH);
	}
	torus_nodes_num = count;
	return TRUE;
}

int create_skip_list(torus_s *torus) {
	/*int i, nodes_num;

	 nodes_num = get_nodes_num(torus->partition);
	 for (i = 0; i < nodes_num; ++i) {
	 insert_skip_list_node(&torus->node_list[i].info);
	 }*/
	return TRUE;
}

int update_skip_list(skip_list *list) {

	skip_list_node *cur_sln;
	cur_sln = list->header->level[0].forward;
	while (cur_sln != NULL) {
		int i, nodes_num, index = 0;
		nodes_num = (cur_sln->height + 1) * 2;
		node_info nodes[nodes_num];
		for (i = 0; i <= cur_sln->height; ++i) {
			if (cur_sln->level[i].forward) {
				nodes[index++] = cur_sln->level[i].forward->leader;
			} else {
				init_node_info(&nodes[index++]);
			}
			if (cur_sln->level[i].backward) {
				nodes[index++] = cur_sln->level[i].backward->leader;
			} else {
				init_node_info(&nodes[index++]);
			}
		}

		char dst_ip[IP_ADDR_LENGTH];
		strncpy(dst_ip, cur_sln->leader.ip, IP_ADDR_LENGTH);
		send_nodes_info(UPDATE_SKIP_LIST, dst_ip, nodes_num, nodes);

		cur_sln = cur_sln->level[0].forward;
	}
	return TRUE;
}

int traverse_skip_list(const char *entry_ip) {
	int socketfd;

	socketfd = new_client_socket(entry_ip);
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
	msg.op = TRAVERSE_SKIP_LIST;
	strncpy(msg.src_ip, local_ip, IP_ADDR_LENGTH);
	strncpy(msg.dst_ip, entry_ip, IP_ADDR_LENGTH);
	memset(msg.stamp, 0, STAMP_SIZE);
	memset(msg.data, 0, DATA_SIZE);

	send_message(socketfd, msg);
	close(socketfd);
	return TRUE;
}

int insert_skip_list_node(skip_list *list, node_info *node_ptr) {
	int i, index, new_level, update_num;
	char src_ip[IP_ADDR_LENGTH], dst_ip[IP_ADDR_LENGTH];
	skip_list_node *sln_ptr, *new_sln;

	node_info *update_nodes, null_node;
	init_node_info(&null_node);

	memset(src_ip, 0, IP_ADDR_LENGTH);
	if (FALSE == get_local_ip(src_ip)) {
		return FALSE;
	}

	// random choose a level for new skip list node
	new_level = random_level();
	new_sln = new_skip_list_node(new_level, node_ptr);
	if (new_sln == NULL) {
		return FALSE;
	}

	// send message to new node to create a new skip list node
	struct message msg;
	msg.op = NEW_SKIP_LIST;
	strncpy(msg.src_ip, src_ip, IP_ADDR_LENGTH);
	strncpy(msg.dst_ip, node_ptr->ip, IP_ADDR_LENGTH);
	strncpy(msg.stamp, "", STAMP_SIZE);
	memcpy(msg.data, &new_level, sizeof(int));
	memcpy(msg.data + sizeof(int), node_ptr, sizeof(node_info));
	if (TRUE == forward_message(msg, 1)) {
		printf("new skip list node %s ... success\n", node_ptr->ip);
	} else {
		printf("new skip list node %s ... failed\n", node_ptr->ip);
		return FALSE;
	}

	// update skip list header's level
	if (new_level > list->level) {
		list->level = new_level;
	}

	update_num = (new_level + 1) * 2;
	update_nodes = (node_info *) malloc(sizeof(node_info) * update_num);

	sln_ptr = list->header;
	index = 0;
	for (i = list->level; i >= 0; --i) {
		while ((sln_ptr->level[i].forward != NULL)
				&& (-1
						== compare(sln_ptr->level[i].forward->leader.dims[2],
								node_ptr->dims[2]))) {
			sln_ptr = sln_ptr->level[i].forward;
		}

		if (i <= new_level) {
			// update new skip list node's forward field
			new_sln->level[i].forward = sln_ptr->level[i].forward;
			update_nodes[index++] =
					(sln_ptr->level[i].forward == NULL) ?
							null_node : sln_ptr->level[i].forward->leader;

			// update new skip list node's backward field
			new_sln->level[i].backward =
					(sln_ptr == list->header) ? NULL : sln_ptr;
			update_nodes[index++] =
					(sln_ptr == list->header) ? null_node : sln_ptr->leader;

			// if current skip list node's forward field exist(not the tail node)
			// update it's(sln_ptr->level[i].forward) backward field
			if (sln_ptr->level[i].forward) {
				new_sln->level[i].forward->level[i].backward = new_sln;

				msg.op = UPDATE_BACKWARD;
				get_node_ip(sln_ptr->level[i].forward->leader, dst_ip);
				strncpy(msg.dst_ip, dst_ip, IP_ADDR_LENGTH);
				memcpy(msg.data, &i, sizeof(int));
				memcpy(msg.data + sizeof(int), (void *) node_ptr,
						sizeof(struct node_info));
				if (TRUE == forward_message(msg, 1)) {
					printf("update skip list node %s's backward ... success\n",
							dst_ip);
				} else {
					printf("update skip list node %s's backward ... failed\n",
							dst_ip);
					return FALSE;
				}
			}

			// update current skip list node's forward field
			sln_ptr->level[i].forward = new_sln;
			if (get_cluster_id(sln_ptr->leader) != -1) {

				msg.op = UPDATE_FORWARD;
				get_node_ip(sln_ptr->leader, dst_ip);
				strncpy(msg.dst_ip, dst_ip, IP_ADDR_LENGTH);
				memcpy(msg.data, &i, sizeof(int));
				memcpy(msg.data + sizeof(int), (void *) node_ptr,
						sizeof(struct node_info));
				if (TRUE == forward_message(msg, 1)) {
					printf("update skip list node %s's forward ... success\n",
							dst_ip);
				} else {
					printf("update skip list node %s's forward ... failed\n",
							dst_ip);
					return FALSE;
				}
			}
		}
	}

	if (FALSE
			== send_nodes_info(UPDATE_SKIP_LIST, node_ptr->ip, update_num,
					update_nodes)) {
		return FALSE;
	}

	return TRUE;
}

int search_skip_list_node(interval interval[], const char *entry_ip) {

	// get local ip address
	char local_ip[IP_ADDR_LENGTH];
	memset(local_ip, 0, IP_ADDR_LENGTH);
	if (FALSE == get_local_ip(local_ip)) {
		return FALSE;
	}

	struct message msg;
	int count = 0;
	msg.op = SEARCH_SKIP_LIST_NODE;
	strncpy(msg.src_ip, local_ip, IP_ADDR_LENGTH);
	strncpy(msg.dst_ip, entry_ip, IP_ADDR_LENGTH);
	memcpy(msg.stamp, "", STAMP_SIZE);
	memcpy(msg.data, &count, sizeof(int));
	memcpy(msg.data + sizeof(int), (void *) interval, DATA_SIZE);

	int i;
	if (FALSE == forward_message(msg, 1)) {
		printf("!!!ERROR!!! ");
		for (i = 0; i < MAX_DIM_NUM; ++i) {
			printf("[%.15f, %.15f] ", interval[i].low, interval[i].high);
		}
	}
	printf("\n");

	return TRUE;
}

int main(int argc, char **argv) {
	/*if (argc < 4) {
	 printf("usage: %s x y z\n", argv[0]);
	 exit(1);
	 }*/

	if (FALSE == read_torus_ip_list()) {
		exit(1);
	}

	cluster_list = new_torus_cluster();

	if (NULL == cluster_list) {
		exit(1);
	}

	// create a new skip list
	slist = new_skip_list(MAXLEVEL);
	if (NULL == slist) {
		exit(1);
	}

	char entry_ip[IP_ADDR_LENGTH];

	// create a new torus by torus partition info
	struct torus_s *torus_ptr;
	torus_ptr = create_torus(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]));
	if (torus_ptr == NULL) {
		exit(1);
	}
	//print_torus(torus_ptr);

	node_info *leader_node;
	leader_node = assign_torus_leader(torus_ptr);

	strncpy(entry_ip, leader_node->ip, IP_ADDR_LENGTH);

	// insert newly created torus cluster into cluster_list
	insert_torus_cluster(cluster_list, torus_ptr);

	//print_torus_cluster(cluster_list);

	if (TRUE == update_torus(torus_ptr)) {
		if (TRUE == insert_skip_list_node(slist, leader_node)) {
			print_skip_list(slist);
			//printf("traverse skip list success!\n");
		}
	}

	printf("\n\n");
	print_torus_cluster(cluster_list);
	printf("\n\n");

	FILE *fp = fopen("./range_query", "rb");
	if (fp == NULL) {
		printf("can't open file\n");
		exit(1);
	}

	int count = 0, i;
	while (count++ < 100) {
		struct interval intval[MAX_DIM_NUM];

		printf("%d.begin search: ", count);
		for (i = 0; i < MAX_DIM_NUM; i++) {
			fscanf(fp, "%lf %lf", &intval[i].low, &intval[i].high);
			printf("[%.15f, %.15f] ", intval[i].low, intval[i].high);
		}
		printf("\n");

		search_skip_list_node(intval, entry_ip);

		printf("search finish.\n");
		//sleep(1);
	}
	fclose(fp);

	return 0;
}

