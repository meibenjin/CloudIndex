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

// torus node list
torus_node *torus_node_list;

// partitions on 3-dimension
torus_partitions torus_p;

int set_partitions(int p_x, int p_y, int p_z) {
	if ((p_x < 1) || (p_y < 1) || (p_z < 1)) {
		printf("the number of partitions too small.\n");
		return FALSE;
	}
	if ((p_x > 100) || (p_y > 100) || (p_z > 100)) {
		printf("the number of partitions too large.\n");
		return FALSE;
	}

	torus_p.p_x = p_x;
	torus_p.p_y = p_y;
	torus_p.p_z = p_z;

	return TRUE;
}

void update_partitions(int direction, struct torus_partitions t_p) {
	switch (direction) {
	case X_R:
	case X_L:
		torus_p.p_x += t_p.p_x;
		break;
	case Y_R:
	case Y_L:
		torus_p.p_y += t_p.p_y;
		break;
	case Z_R:
	case Z_L:
		torus_p.p_z += t_p.p_z;
		break;
	}
}

int get_nodes_num(struct torus_partitions t_p) {
	return t_p.p_x * t_p.p_y * t_p.p_z;
}

int translate_coordinates(int direction, struct torus_partitions t_p,
		struct torus_node *node_list) {
	if (node_list == NULL) {
		printf("translate_coordinates: node_list is null pointer.\n");
		return FALSE;
	}
	int nodes_num = get_nodes_num(t_p);
	int i, x, y, z;
	for (i = 0; i < nodes_num; ++i) {
		struct coordinate c = get_node_id(node_list[i]);
		switch (direction) {
		case X_R:
		case X_L:
			set_node_id(&node_list[i], t_p.p_x + c.x, c.y, c.z);
			break;
		case Y_R:
		case Y_L:
			set_node_id(&node_list[i], c.x, t_p.p_y + c.y, c.z);
			break;
		case Z_R:
		case Z_L:
			set_node_id(&node_list[i], c.x, c.y, t_p.p_z + c.z);
			break;
		}
	}
	return TRUE;
}

int assign_node_ip(torus_node *node_ptr) {
	char ip[8][20] = { "10.77.30.101", "10.77.30.199", "10.77.30.200",
			"10.77.30.201", "10.77.30.202", "10.77.30.203", "10.77.30.204",
			"10.77.30.205" };
	static int i = 0;
	if (!node_ptr) {
		printf("assign_node_ip: node_ptr is null pointer.\n");
		return FALSE;
	}
	set_node_ip(node_ptr, ((i < 8) ? ip[i++] : "0.0.0.0"));

	return TRUE;
}

int assign_cluster_id(torus_node *node_ptr) {
	if (!node_ptr) {
		printf("assign_cluster_id: node_ptr is null pointer.\n");
		return FALSE;
	}

	static int index = 0;
	set_cluster_id(node_ptr, index++);
	return TRUE;
}

int get_node_index(int x, int y, int z) {
	return x * torus_p.p_y * torus_p.p_z + y * torus_p.p_z + z;
}

int set_neighbors(torus_node *node_ptr) {
	if (!node_ptr) {
		printf("set_neighbors: node_ptr is null pointer.\n");
		return FALSE;
	}
	struct coordinate c = get_node_id(*node_ptr);

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
	int xri = get_node_index(xrc, c.y, c.z);
	// index of the left neighbor of node_ptr on x-axis
	int xli = get_node_index(xlc, c.y, c.z);

	// index of the right neighbor of node_ptr on y-axis
	int yri = get_node_index(c.x, yrc, c.z);
	// index of the left neighbor of node_ptr on y-axis
	int yli = get_node_index(c.x, ylc, c.z);

	// index of the right neighbor of node_ptr on z-axis
	int zri = get_node_index(c.x, c.y, zrc);
	// index of the left neighbor of node_ptr on z-axis
	int zli = get_node_index(c.x, c.y, zlc);

	int i = 0;
	if (xrc != c.x) {
		node_ptr->neighbors[i++] = &torus_node_list[xri];
	}
	if (xri != xli) {
		node_ptr->neighbors[i++] = &torus_node_list[xli];
	}

	if (yrc != c.y) {
		node_ptr->neighbors[i++] = &torus_node_list[yri];
	}
	if (yri != yli) {
		node_ptr->neighbors[i++] = &torus_node_list[yli];
	}

	if (zrc != c.z) {
		node_ptr->neighbors[i++] = &torus_node_list[zri];
	}
	if (zri != zli) {
		node_ptr->neighbors[i++] = &torus_node_list[zli];
	}

	set_neighbors_num(node_ptr, i);

	return TRUE;
}

int new_torus(struct torus_partitions new_torus_p, struct torus_node *new_list) {
	if (new_list == NULL) {
		printf("create_torus: new_list is null pointer.\n");
		return FALSE;
	}

	int i, j, k, index, new_nodes_num;
	new_nodes_num = get_nodes_num(new_torus_p);

	index = 0;
	for (i = 0; i < new_torus_p.p_x; ++i) {
		for (j = 0; j < new_torus_p.p_y; ++j) {
			for (k = 0; k < new_torus_p.p_z; ++k) {
				if (index >= new_nodes_num) {
					return FALSE;
				}
				torus_node *new_node = &new_list[index];

				init_torus_node(new_node);
				if (assign_node_ip(new_node) == FALSE) {
					return FALSE;
				}
				// TODO only for skip list test
				assign_cluster_id(new_node);
				set_node_id(new_node, i, j, k);
				index++;
			}
		}
	}

	return TRUE;
}

int construct_torus() {
	int i;
	int nodes_num = get_nodes_num(torus_p);

	torus_node_list = (torus_node *) malloc(sizeof(torus_node) * nodes_num);
	if (!torus_node_list) {
		printf("malloc torus node list failed.\n");
		return FALSE;
	}
	// create a new torus
	if (FALSE == new_torus(torus_p, torus_node_list)) {
		printf("construct_torus: create a new torus failed.\n");
		free(torus_node_list);
		return FALSE;
	}

	for (i = 0; i < nodes_num; ++i) {
		set_neighbors(&torus_node_list[i]);
	}
	return TRUE;
}

int append_torus(int direction) {
	// TODO append_torus_p can specified by user
	struct torus_partitions append_torus_p;
	append_torus_p = torus_p;

	int append_nodes_num = get_nodes_num(append_torus_p);
	struct torus_node *append_list;
	append_list = (torus_node *) malloc(sizeof(torus_node) * append_nodes_num);

	if (FALSE == new_torus(append_torus_p, append_list)) {
		printf("append_torus: append torus failed.\n");
		if (!append_list) {
			free(append_list);
		}
		return FALSE;
	}

	// merge current torus with newly appended torus cluster
	if (FALSE == merge_torus(direction, append_torus_p, append_list)) {
		printf("append_torus: append torus nodes failed\n");
		if (!append_list) {
			free(append_list);
		}
		return FALSE;
	}

	return TRUE;
}

int merge_torus(int direction, struct torus_partitions append_torus_p,
		struct torus_node *append_list) {
	if (append_list == NULL) {
		printf("merge_torus: src_list is null pointer.\n");
		return FALSE;
	}

	switch (direction) {
	case X_R:
	case Y_R:
	case Z_R:
		translate_coordinates(direction, append_torus_p, append_list);
		break;
	case X_L:
	case Y_L:
	case Z_L:
		translate_coordinates(direction, torus_p, torus_node_list);
		break;
	}

	int i, index, ori_num, append_num, merged_num;
	struct coordinate c;
	struct torus_node *new_list;

	ori_num = get_nodes_num(torus_p);
	append_num = get_nodes_num(append_torus_p);

	new_list = (torus_node *) malloc(
			sizeof(torus_node) * (ori_num + append_num));
	if (!new_list) {
		printf("malloc torus node list failed.\n");
		// TODO do something when append torus nodes failed
		return FALSE;
	}

	// important: update torus_p
	update_partitions(direction, append_torus_p);

	for (i = 0; i < ori_num; ++i) {
		c = get_node_id(torus_node_list[i]);
		index = get_node_index(c.x, c.y, c.z);
		new_list[index] = torus_node_list[i];
	}

	for (i = 0; i < append_num; ++i) {
		c = get_node_id(append_list[i]);
		index = get_node_index(c.x, c.y, c.z);
		new_list[index] = append_list[i];
	}

	// free origin torus_node_list and append_list
	if (!torus_node_list) {
		free(torus_node_list);
	}
	if (!append_list) {
		free(append_list);
	}

	// point torus_node_list to merged new_list
	torus_node_list = new_list;
	merged_num = get_nodes_num(torus_p);
	for (i = 0; i < merged_num; ++i) {
		set_neighbors(&torus_node_list[i]);
	}

	return TRUE;
}

int send_torus_nodes(const char *dst_ip, int nodes_num, struct node_info *nodes) {
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
	msg.op = UPDATE_TORUS;
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
				printf("%s: send torus nodes ...... finish.\n", dst_ip);
				ret = TRUE;
			} else {
				printf("%s: send torus nodes ...... error occurred.\n", dst_ip);
				ret = FALSE;
			}
		} else {
			ret = FALSE;
		}
	} else {
		printf("%s: send torus nodes ...... failed.\n", dst_ip);
		ret = FALSE;
	}

	close(socketfd);
	return ret;
}

int update_torus() {
	int i, j, index, nodes_num;
	if (torus_node_list == NULL) {
		printf("update_torus: torus_node_list is null.\n");
		return FALSE;
	}

	nodes_num = get_nodes_num(torus_p);
	for (i = 0; i < nodes_num; ++i) {
		int neighbors_num = get_neighbors_num(torus_node_list[i]);

		// the nodes array includes dst nodes and its' neighbors
		struct node_info nodes[neighbors_num + 1];

		// add dst node itself
		nodes[0] = torus_node_list[i].info;

		struct torus_node *node_ptr = NULL;
		node_ptr = &torus_node_list[i];
		for (j = 0; j < neighbors_num; ++j) {
			if (node_ptr && node_ptr->neighbors[j]) {
				nodes[j + 1] = node_ptr->neighbors[j]->info;
				index++;
			}
		}

		char dst_ip[IP_ADDR_LENGTH];
		memset(dst_ip, 0, IP_ADDR_LENGTH);
		get_node_ip(*node_ptr, dst_ip);

		// send to dst_ip
		if (FALSE == send_torus_nodes(dst_ip, neighbors_num + 1, nodes)) {
			return FALSE;
		}
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
	msg.op = TRAVERSE;
	strncpy(msg.src_ip, local_ip, IP_ADDR_LENGTH);
	strncpy(msg.dst_ip, entry_ip, IP_ADDR_LENGTH);
	memset(msg.stamp, 0, STAMP_SIZE);
	memset(msg.data, 0, DATA_SIZE);

	send_message(socketfd, msg);
	close(socketfd);
	return TRUE;
}

void print_torus() {
	int i, nodes_num;
	nodes_num = get_nodes_num(torus_p);
	for (i = 0; i < nodes_num; ++i) {
		print_torus_node(torus_node_list[i]);
	}
}

int create_skip_list() {
	if (FALSE == init_skip_list()) {
		return FALSE;
	}

	int i, nodes_num;
	nodes_num = get_nodes_num(torus_p);
	for (i = 0; i < nodes_num; ++i) {
		if(FALSE == insert_skip_list(torus_node_list[i])){
			// TODO do something when insert failed
			continue;
		}
	}

	if(TRUE ==search_skip_list(torus_node_list[6])){
		printf("find\n");
	}

	traverse_skip_list();
	return TRUE;
}

int main(int argc, char **argv) {
	if (argc < 4) {
		printf("usage: %s x y z\n", argv[0]);
		exit(1);
	}

	// set 3-dimension partitions
	if (set_partitions(atoi(argv[1]), atoi(argv[2]), atoi(argv[3])) == FALSE) {
		exit(1);
	}

	if (FALSE == construct_torus()) {
		exit(1);
	}

	/*int direction = X_L;
	 if( FALSE == append_torus(direction)) {
	 // re-construct the original torus when append torus failed
	 construct_torus();
	 }*/
	print_torus();

	create_skip_list();

	/*if (TRUE == update_torus()) {
	 while (1) {
	 char entry_ip[IP_ADDR_LENGTH];
	 printf("input entry ip:");
	 scanf("%s", entry_ip);
	 traverse_torus(entry_ip);
	 }
	 }*/

	return 0;
}

