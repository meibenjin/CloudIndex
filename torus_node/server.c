/*
 * server.c
 *
 *  Created on: Sep 24, 2013
 *      Author: meibenjin
 */

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>

#include"server.h"
#include"logs/log.h"
#include"skip_list/skip_list.h"

__asm__(".symver memcpy,memcpy@GLIBC_2.2.5");

char result_ip[IP_ADDR_LENGTH] = "172.16.0.83";

struct query {
	char stamp[STAMP_SIZE];
	struct timespec start;
};

struct query query_list[10000];

request *new_request() {
	request *req_ptr;
	req_ptr = (struct request *) malloc(sizeof(request));
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

request *find_request(request *list, const char *req_stamp) {
	request *req_ptr = list->next;
	while (req_ptr) {
		if (strcmp(req_ptr->stamp, req_stamp) == 0) {
			return req_ptr;
		}
		req_ptr = req_ptr->next;
	}
	return NULL;
}

request *insert_request(request *list, const char *req_stamp) {
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

int remove_request(request *list, const char *req_stamp) {
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

int do_traverse_torus(struct message msg) {
	char stamp[STAMP_SIZE];
	memset(stamp, 0, STAMP_SIZE);

	int neighbors_num = get_neighbors_num(the_torus_node);

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

		//write log
		char buf[1024];
		memset(buf, 0, 1024);
		sprintf(buf, "traverse torus: %s -> %s\n", msg.src_ip, msg.dst_ip);
		write_log(TORUS_NODE_LOG, buf);

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
	int i;
	char src_ip[IP_ADDR_LENGTH], dst_ip[IP_ADDR_LENGTH];

	int neighbors_num = get_neighbors_num(the_torus_node);

	get_node_ip(the_torus_node.info, src_ip);
	for (i = 0; i < neighbors_num; ++i) {
		get_node_ip(the_torus_node.neighbors[i]->info, dst_ip);

		strncpy(msg.src_ip, src_ip, IP_ADDR_LENGTH);
		strncpy(msg.dst_ip, dst_ip, IP_ADDR_LENGTH);
		forward_message(msg, 0);
	}
	return TRUE;
}

int do_update_torus(struct message msg) {
	int i;
	int nodes_num = 0;

	memcpy(&nodes_num, msg.data, sizeof(int));
	if (nodes_num <= 0) {
		printf("update_torus_node: torus nodes number is wrong\n");
		return FALSE;
	}

	node_info nodes[nodes_num];

	for (i = 0; i < nodes_num; ++i) {
		memcpy(&nodes[i],
				(void*) (msg.data + sizeof(int) + sizeof(node_info) * i),
				sizeof(node_info));
	}

	// construct a torus node from several nodes
	the_torus_node.info = nodes[0];
	set_neighbors_num(&the_torus_node, nodes_num - 1);
	for (i = 1; i < nodes_num; ++i) {
		torus_node *new_node = new_torus_node();
		if (new_node == NULL) {
			return FALSE;
		}
		new_node->info = nodes[i];
		the_torus_node.neighbors[i - 1] = new_node;
	}
	return TRUE;
}

int do_update_partition(struct message msg) {

	memcpy(&the_partition, msg.data, sizeof(struct torus_partitions));

	char buf[1024];
	sprintf(buf, "torus partitions:[%d %d %d]\n", the_partition.p_x,
			the_partition.p_y, the_partition.p_z);
	write_log(TORUS_NODE_LOG, buf);

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
	sln_ptr = the_skip_list.header;

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
	cur_sln = the_skip_list.header;
	forward = cur_sln->level[0].forward;
	backward = cur_sln->level[0].backward;

	write_log(TORUS_NODE_LOG, "visit myself:");
	print_node_info(cur_sln->leader);

	get_node_ip(cur_sln->leader, src_ip);
	strncpy(msg.src_ip, src_ip, IP_ADDR_LENGTH);
	if (strcmp(msg.data, "") == 0) {
		if (forward) {
			get_node_ip(forward->leader, dst_ip);
			strncpy(msg.dst_ip, dst_ip, IP_ADDR_LENGTH);
			strncpy(msg.data, "forward", DATA_SIZE);
			forward_message(msg, 1);
		}

		if (backward) {
			get_node_ip(backward->leader, dst_ip);
			strncpy(msg.dst_ip, dst_ip, IP_ADDR_LENGTH);
			strncpy(msg.data, "backward", DATA_SIZE);
			forward_message(msg, 1);
		}

	} else if (strcmp(msg.data, "forward") == 0) {
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
	// analysis node info from message
	memcpy(&i, msg.data, sizeof(int));

	node_info f_node, b_node;
	memcpy(&f_node, (void*) (msg.data + sizeof(int)), sizeof(node_info));
	memcpy(&b_node, (void*) (msg.data + sizeof(int) + sizeof(node_info)),
			sizeof(node_info));

	skip_list_node *sln_ptr, *new_sln;
	sln_ptr = the_skip_list.header;
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
	// analysis node info from message
	memcpy(&i, msg.data, sizeof(int));

	node_info f_node;
	memcpy(&f_node, (void*) (msg.data + sizeof(int)), sizeof(node_info));

	skip_list_node *sln_ptr, *new_sln;
	sln_ptr = the_skip_list.header;
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
	// analysis node info from message
	memcpy(&i, msg.data, sizeof(int));

	node_info b_node;
	memcpy(&b_node, (void*) (msg.data + sizeof(int)), sizeof(node_info));

	skip_list_node *sln_ptr, *new_sln;
	sln_ptr = the_skip_list.header;
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
	skip_list_node *sln_ptr;
	// analysis node info from message
	memcpy(&level, msg.data, sizeof(int));
	node_info leader;
	memcpy(&leader, msg.data + sizeof(int), sizeof(node_info));

	the_skip_list = *new_skip_list(level);
	sln_ptr = the_skip_list.header;
	sln_ptr->leader = leader;
	sln_ptr->height = level;
	the_skip_list.level = level;
	return TRUE;
}

int forward_search(struct message msg, int d) {
	int i;
	char buf[1024], src_ip[IP_ADDR_LENGTH], dst_ip[IP_ADDR_LENGTH];
	interval interval[MAX_DIM_NUM];
	memcpy(interval, msg.data + sizeof(int), sizeof(interval) * MAX_DIM_NUM);

	struct coordinate lower_id = get_node_id(the_torus_node.info);
	struct coordinate upper_id = get_node_id(the_torus_node.info);
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

	node_info *lower_neighbor = get_neighbor_by_id(the_torus_node, lower_id);
	node_info *upper_neighbor = get_neighbor_by_id(the_torus_node, upper_id);

	if (strcmp(lower_neighbor->ip, msg.src_ip) == 0) {
		lower_neighbor = NULL;
	}
	if (strcmp(upper_neighbor->ip, msg.src_ip) == 0) {
		upper_neighbor = NULL;
	}

	int len = 0;
	len = sprintf(buf, "query:");
	for (i = 0; i < MAX_DIM_NUM; ++i) {
		len += sprintf(buf + len, "[%d, %d] ", interval[i].low,
				interval[i].high);
	}
	sprintf(buf + len, "\n");

	// if current torus node is on the lower end
	if ((the_torus_node.info.dims[d].low < lower_neighbor->dims[d].low)
			&& (the_torus_node.info.dims[d].low < upper_neighbor->dims[d].low)) {
		if (get_distance(interval[d], lower_neighbor->dims[d])
				< get_distance(interval[d], upper_neighbor->dims[d])) {
			upper_neighbor = lower_neighbor;
		}
		lower_neighbor = NULL;

	} else if ((the_torus_node.info.dims[d].high > lower_neighbor->dims[d].high)
			&& (the_torus_node.info.dims[d].high > upper_neighbor->dims[d].high)) {
		if (get_distance(interval[d], upper_neighbor->dims[d])
				< get_distance(interval[d], lower_neighbor->dims[d])) {
			lower_neighbor = upper_neighbor;
		}
		upper_neighbor = NULL;
	}

	if ((lower_neighbor != NULL)
			&& (interval[d].low < the_torus_node.info.dims[d].low)) {
		struct message new_msg;
		get_node_ip(the_torus_node.info, src_ip);
		get_node_ip(*lower_neighbor, dst_ip);
		fill_message(msg.op, src_ip, dst_ip, msg.stamp, msg.data, &new_msg);

		forward_message(new_msg, 0);
	}
	if ((upper_neighbor != NULL)
			&& (interval[d].high > the_torus_node.info.dims[d].high)) {
		struct message new_msg;
		get_node_ip(the_torus_node.info, src_ip);
		get_node_ip(*upper_neighbor, dst_ip);
		fill_message(msg.op, src_ip, dst_ip, msg.stamp, msg.data, &new_msg);

		forward_message(new_msg, 0);
	}
	return TRUE;
}

int do_search_torus_node(struct message msg) {
	int i;
	char buf[1024];
	//char src_ip[IP_ADDR_LENGTH], dst_ip[IP_ADDR_LENGTH];

	interval interval[MAX_DIM_NUM];
	char stamp[STAMP_SIZE];
	int count;
	memcpy(&count, msg.data, sizeof(int));
	count++;
	memcpy(msg.data, &count, sizeof(int));
	memcpy(interval, msg.data + sizeof(int), sizeof(interval) * MAX_DIM_NUM);
	memcpy(stamp, msg.stamp, STAMP_SIZE);

	request *req_ptr = find_request(req_list, stamp);

	if (NULL == req_ptr) {
		req_ptr = insert_request(req_list, stamp);

		if (1 == overlaps(interval, the_torus_node.info.dims)) {
			message new_msg;
			// only for test
			//when receive query and search skip list node finish send message to collect-result node
			fill_message(RECEIVE_RESULT, msg.dst_ip, result_ip, msg.stamp,
					msg.data, &new_msg);

			forward_message(new_msg, 0);// end test

			write_log(TORUS_NODE_LOG, "search torus success!\n\t");
			print_node_info(the_torus_node.info);

			int len = 0;
			len = sprintf(buf, "query:");
			for (i = 0; i < MAX_DIM_NUM; ++i) {
				len += sprintf(buf + len, "[%d, %d] ", interval[i].low,
						interval[i].high);
			}
			sprintf(buf + len, "\n");
			write_log(CTRL_NODE_LOG, buf);

			len = 0;
			len = sprintf(buf, "%s:", the_torus_node.info.ip);
			for (i = 0; i < MAX_DIM_NUM; ++i) {
				len += sprintf(buf + len, "[%d, %d] ",
						the_torus_node.info.dims[i].low,
						the_torus_node.info.dims[i].high);
			}
			sprintf(buf + len, "\n%s:search torus success. %d\n\n", msg.dst_ip,
					count);
			write_log(CTRL_NODE_LOG, buf);

			for (i = 0; i < MAX_DIM_NUM; i++) {
				forward_search(msg, i);
			}
			//TODO do rtree search

		} else {
			for (i = 0; i < MAX_DIM_NUM; i++) {
				if (interval_overlap(interval[i], the_torus_node.info.dims[i])
						!= 0) {
					forward_search(msg, i);
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
	char buf[1024], src_ip[IP_ADDR_LENGTH], dst_ip[IP_ADDR_LENGTH];
	interval interval[MAX_DIM_NUM];
	char stamp[STAMP_SIZE];
	//int count;
	//memcpy(&count, msg.data, sizeof(int));
	//count++;
	//memcpy(msg.data, &count, sizeof(int));
	memcpy(interval, msg.data + sizeof(int), sizeof(interval) * MAX_DIM_NUM);

	message new_msg;

	//get current skip list node  ip address
	get_node_ip(the_skip_list.header->leader, src_ip);

	skip_list_node *sln_ptr;
	sln_ptr = the_skip_list.header;

	if (0 == interval_overlap(sln_ptr->leader.dims[2], interval[2])) { // node searched
		if (FALSE == gen_request_stamp(stamp)) {
			return FALSE;
		}

		// only for test
		//when receive query and search skip list node finish send message to collect-result node
		fill_message(RECEIVE_QUERY, src_ip, result_ip, stamp, "", &new_msg);
		forward_message(new_msg, 0); // end test

		int len = 0;
		len = sprintf(buf, "query:");
		for (i = 0; i < MAX_DIM_NUM; ++i) {
			len += sprintf(buf + len, "[%d, %d] ", interval[i].low,
					interval[i].high);
		}
		sprintf(buf + len, "\n");
		write_log(TORUS_NODE_LOG, buf);

		write_log(TORUS_NODE_LOG,
				"search skip list success! turn to search torus\n");

		// turn to torus layer
		msg.op = SEARCH_TORUS_NODE;
		strncpy(msg.stamp, stamp, STAMP_SIZE);
		do_search_torus_node(msg);

		//decide whether forward message to it's forward and backward
		if (strcmp(msg.stamp, "") == 0) {
			if ((sln_ptr->level[0].forward != NULL)
					&& (interval_overlap(
							sln_ptr->level[0].forward->leader.dims[2],
							interval[2]) == 0)) {
				get_node_ip(sln_ptr->level[0].forward->leader, dst_ip);
				fill_message(msg.op, src_ip, dst_ip, "forward", msg.data,
						&new_msg);
				forward_message(new_msg, 1);
			}

			if ((sln_ptr->level[0].backward != NULL)
					&& (interval_overlap(
							sln_ptr->level[0].backward->leader.dims[2],
							interval[2]) == 0)) {
				get_node_ip(sln_ptr->level[0].backward->leader, dst_ip);
				fill_message(msg.op, src_ip, dst_ip, "backward", msg.data,
						&new_msg);
				forward_message(new_msg, 1);
			}
		} else if (strcmp(msg.stamp, "forward") == 0) {
			if ((sln_ptr->level[0].forward != NULL)
					&& (interval_overlap(
							sln_ptr->level[0].forward->leader.dims[2],
							interval[2]) == 0)) {
				get_node_ip(sln_ptr->level[0].forward->leader, dst_ip);
				fill_message(msg.op, src_ip, dst_ip, "forward", msg.data,
						&new_msg);
				forward_message(new_msg, 1);
			}
		} else {
			if ((sln_ptr->level[0].backward != NULL)
					&& (interval_overlap(
							sln_ptr->level[0].backward->leader.dims[2],
							interval[2]) == 0)) {
				get_node_ip(sln_ptr->level[0].backward->leader, dst_ip);
				fill_message(msg.op, src_ip, dst_ip, "backward", msg.data,
						&new_msg);
				forward_message(new_msg, 1);
			}
		}

	} else if (-1 == interval_overlap(sln_ptr->leader.dims[2], interval[2])) { // node is on the forward of skip list
		int visit_forward = 0;
		for (i = the_skip_list.level; i >= 0; --i) {
			if ((sln_ptr->level[i].forward != NULL)
					&& (interval_overlap(
							sln_ptr->level[i].forward->leader.dims[2],
							interval[2]) <= 0)) {

				get_node_ip(sln_ptr->level[i].forward->leader, dst_ip);
				fill_message(msg.op, src_ip, dst_ip, "forward", msg.data,
						&new_msg);
				forward_message(new_msg, 1);
				visit_forward = 1;
				break;
			}
		}
		if (visit_forward == 0) {
			write_log(TORUS_NODE_LOG, "search failed!\n");
		}

	} else {							// node is on the backward of skip list
		int visit_backward = 0;
		for (i = the_skip_list.level; i >= 0; --i) {
			if ((sln_ptr->level[i].backward != NULL)
					&& (interval_overlap(
							sln_ptr->level[i].backward->leader.dims[2],
							interval[2]) >= 0)) {

				get_node_ip(sln_ptr->level[i].backward->leader, dst_ip);
				fill_message(msg.op, src_ip, dst_ip, "backward", msg.data,
						&new_msg);
				forward_message(new_msg, 1);
				visit_backward = 1;
				break;
			}
		}
		if (visit_backward == 0) {
			write_log(TORUS_NODE_LOG, "search failed!\n");
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
	struct timespec query_start, query_end;
	clock_gettime(CLOCK_REALTIME, &query_end);

	char stamp[STAMP_SIZE];
	char ip[IP_ADDR_LENGTH];
	int i, count;
	long qtime;
	interval intval[MAX_DIM_NUM];
	memcpy(stamp, msg.stamp, STAMP_SIZE);
	memcpy(ip, msg.src_ip, IP_ADDR_LENGTH);
	memcpy(&count, msg.data, sizeof(int));
	memcpy(intval, msg.data + sizeof(int), sizeof(interval) * MAX_DIM_NUM);

	for (i = 0; i < 10000; i++) {
		if (strcmp(stamp, query_list[i].stamp) == 0) {
			query_start = query_list[i].start;
			qtime = get_elasped_time(query_start, query_end);
		}
	}

	char file_name[1024];
	sprintf(file_name, "/root/result/%s", stamp);
	FILE *fp = fopen(file_name, "ab+");
	if (fp == NULL) {
		printf("can't open file %s\n", file_name);
		return FALSE;
	}

	fprintf(fp, "query:");
//	fprintf(stdout, "query:");
	for (i = 0; i < MAX_DIM_NUM; i++) {
		fprintf(fp, "[%d, %d] ", intval[i].low, intval[i].high);
		//	fprintf(stdout, "[%d, %d] ", intval[i].low, intval[i].high);
	}
	fprintf(fp, "\n");
	//fprintf(stdout, "\n");

	//fprintf(fp, "start time:[%ld:%ld]\n", query_start.tv_sec, query_start.tv_nsec);
	//fprintf(fp, "end time:[%ld:%ld]\n", query_end.tv_sec , query_end.tv_nsec);
	fprintf(fp, "query time:%f us\n", (double) qtime / 1000.0);
	//fprintf(stdout, "query time:%f ms\n", qtime);

	fprintf(fp, "%s:%d\n\n", ip, count);
	//fprintf(stdout, "%s:%d\n\n", ip, count);
	fclose(fp);

	return TRUE;
}

int do_receive_query(struct message msg) {
	struct timespec query_start;
	clock_gettime(CLOCK_REALTIME, &query_start);
	static int list_index = 0;
	strncpy(query_list[list_index].stamp, msg.stamp, STAMP_SIZE);
	query_list[list_index].start = query_start;
	list_index++;

	char buf[1024];
	sprintf(buf, "index:%d [%ld:%ld]\n", list_index, query_start.tv_sec,
			query_start.tv_nsec);
	write_log(TORUS_NODE_LOG, buf);
	return TRUE;
}

int process_message(int socketfd, struct message msg) {

	//char buf[1024];
	struct reply_message reply_msg;
	int reply_code = SUCCESS;
	switch (msg.op) {

	case UPDATE_TORUS:

		write_log(TORUS_NODE_LOG, "receive request update torus.\n");

		if (FALSE == do_update_torus(msg)) {
			reply_code = FAILED;
		}

		reply_msg.op = msg.op;
		reply_msg.reply_code = reply_code;
		strncpy(reply_msg.stamp, msg.stamp, STAMP_SIZE);

		if (FALSE == send_reply(socketfd, reply_msg)) {
			// TODO handle send reply failed
			return FALSE;
		}
		print_torus_node(the_torus_node);
		break;

	case UPDATE_PARTITION:
		write_log(TORUS_NODE_LOG, "receive request update torus.\n");
		if (FALSE == do_update_partition(msg)) {
			reply_code = FAILED;
		}

		reply_msg.op = msg.op;
		reply_msg.reply_code = reply_code;
		strncpy(reply_msg.stamp, msg.stamp, STAMP_SIZE);

		if (FALSE == send_reply(socketfd, reply_msg)) {
			// TODO handle send reply failed
			return FALSE;
		}
		break;

	case TRAVERSE_TORUS:
		write_log(TORUS_NODE_LOG, "receive request traverse torus.\n");
		do_traverse_torus(msg);
		break;

	case SEARCH_TORUS_NODE:
		write_log(TORUS_NODE_LOG, "\nreceive request search torus node.\n");
		do_search_torus_node(msg);
		break;

	case UPDATE_SKIP_LIST:
		write_log(TORUS_NODE_LOG, "receive request update skip list.\n");

		if (FALSE == do_update_skip_list(msg)) {
			reply_code = FAILED;
		}
		reply_msg.op = msg.op;
		reply_msg.reply_code = reply_code;
		strncpy(reply_msg.stamp, msg.stamp, STAMP_SIZE);
		if (FALSE == send_reply(socketfd, reply_msg)) {
			// TODO handle send reply failed
			return FALSE;
		}
		print_skip_list_node(&the_skip_list);
		break;

	case UPDATE_SKIP_LIST_NODE:
		write_log(TORUS_NODE_LOG, "receive request update skip list node.\n");

		if (FALSE == do_update_skip_list_node(msg)) {
			reply_code = FAILED;
		}
		reply_msg.op = msg.op;
		reply_msg.reply_code = reply_code;
		strncpy(reply_msg.stamp, msg.stamp, STAMP_SIZE);
		if (FALSE == send_reply(socketfd, reply_msg)) {
			// TODO handle send reply failed
			return FALSE;
		}
		print_skip_list_node(&the_skip_list);
		break;

	case SEARCH_SKIP_LIST_NODE:
		write_log(TORUS_NODE_LOG, "\nreceive request search skip list node.\n");

		if (FALSE == do_search_skip_list_node(msg)) {
			reply_code = FAILED;
		}
		reply_msg.op = msg.op;
		reply_msg.reply_code = reply_code;
		strncpy(reply_msg.stamp, msg.stamp, STAMP_SIZE);
		if (FALSE == send_reply(socketfd, reply_msg)) {
			// TODO handle send reply failed
			return FALSE;
		}
		break;

	case UPDATE_FORWARD:
		write_log(TORUS_NODE_LOG,
				"receive request update skip list node's forward field.\n");
		if (FALSE == do_update_forward(msg)) {
			reply_code = FAILED;
		}
		reply_msg.op = msg.op;
		reply_msg.reply_code = reply_code;
		strncpy(reply_msg.stamp, msg.stamp, STAMP_SIZE);
		if (FALSE == send_reply(socketfd, reply_msg)) {
			// TODO handle send reply failed
			return FALSE;
		}
		print_skip_list_node(&the_skip_list);
		break;

	case UPDATE_BACKWARD:
		write_log(TORUS_NODE_LOG,
				"receive request update skip list node's backward field.\n");

		if (FALSE == do_update_backward(msg)) {
			reply_code = FAILED;
		}
		reply_msg.op = msg.op;
		reply_msg.reply_code = reply_code;
		strncpy(reply_msg.stamp, msg.stamp, STAMP_SIZE);
		if (FALSE == send_reply(socketfd, reply_msg)) {
			// TODO handle send reply failed
			return FALSE;
		}
		print_skip_list_node(&the_skip_list);
		break;

	case NEW_SKIP_LIST:
		write_log(TORUS_NODE_LOG, "receive request new skip list.\n");
		if (FALSE == do_new_skip_list(msg)) {
			reply_code = FAILED;
		}
		reply_msg.op = msg.op;
		reply_msg.reply_code = reply_code;
		strncpy(reply_msg.stamp, msg.stamp, STAMP_SIZE);
		if (FALSE == send_reply(socketfd, reply_msg)) {
			// TODO handle send reply failed
			return FALSE;
		}
		print_skip_list_node(&the_skip_list);
		break;

	case TRAVERSE_SKIP_LIST:
		write_log(TORUS_NODE_LOG, "receive request traverse skip list.\n");
		do_traverse_skip_list(msg);
		break;

	case RECEIVE_QUERY:
		printf("receive request receive query.\n");
		do_receive_query(msg);
		break;

	case RECEIVE_RESULT:
		printf("receive request receive result.\n");
		do_receive_result(msg);
		break;

	default:
		reply_code = WRONG_OP;

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

