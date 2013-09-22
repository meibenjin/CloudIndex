/*
 * control.c
 *
 *  Created on: Sep 20, 2013
 *      Author: meibenjin
 */

#include"control.h"

int set_partitions(int p_x, int p_y, int p_z) {
	if ((p_x < 2) || (p_y < 2) || (p_z < 2)) {
		printf("the number of partitions too small.\n");
		return FALSE;
	}
	if ((p_x > 10) || (p_y > 10) || (p_z > 10)) {
		printf("the number of partitions too large.\n");
		return FALSE;
	}

	torus_p.p_x = p_x;
	torus_p.p_y = p_y;
	torus_p.p_z = p_z;

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
	strncpy(node_ptr->info.ip, ((i < 8) ? ip[i++] : "0.0.0.0"),
	IP_ADDR_LENGTH);
	return TRUE;
}

int get_node_index(int x, int y, int z) {
	return x * torus_p.p_y * torus_p.p_z + y * torus_p.p_z + z;
}

int set_neighbors(torus_node *node_ptr, int x, int y, int z) {
	if (!node_ptr) {
		printf("set_neighbors: node_ptr is null pointer.\n");
		return FALSE;
	}

	// x-coordinate of the right neighbor of node_ptr on x-axis
	int xrc = (x + torus_p.p_x + 1) % torus_p.p_x;
	// x-coordinate of the left neighbor of node_ptr on x-axis
	int xlc = (x + torus_p.p_x - 1) % torus_p.p_x;

	// y-coordinate of the right neighbor of node_ptr on y-axis
	int yrc = (y + torus_p.p_y + 1) % torus_p.p_y;
	// y-coordinate of the left neighbor of node_ptr on y-axis
	int ylc = (y + torus_p.p_y - 1) % torus_p.p_y;

	// z-coordinate of the right neighbor of node_ptr on z-axis
	int zrc = (z + torus_p.p_z + 1) % torus_p.p_z;
	// z-coordinate of the left neighbor of node_ptr on z-axis
	int zlc = (z + torus_p.p_z - 1) % torus_p.p_z;

	// index of the right neighbor of node_ptr on x-axis
	int xri = get_node_index(xrc, y, z);
	// index of the left neighbor of node_ptr on x-axis
	int xli = get_node_index(xlc, y, z);

	// index of the right neighbor of node_ptr on y-axis
	int yri = get_node_index(x, yrc, z);
	// index of the left neighbor of node_ptr on y-axis
	int yli = get_node_index(x, ylc, z);

	// index of the right neighbor of node_ptr on z-axis
	int zri = get_node_index(x, y, zrc);
	// index of the left neighbor of node_ptr on z-axis
	int zli = get_node_index(x, y, zlc);

	int i = 0;
	node_ptr->neighbors[i++] = &torus_node_list[xri];
	if (xri != xli) {
		node_ptr->neighbors[i++] = &torus_node_list[xli];
	}
	node_ptr->neighbors[i++] = &torus_node_list[yri];
	if (yri != yli) {
		node_ptr->neighbors[i++] = &torus_node_list[yli];
	}
	node_ptr->neighbors[i++] = &torus_node_list[zri];
	if (zri != zli) {
		node_ptr->neighbors[i++] = &torus_node_list[zli];
	}

	set_neighbors_num(node_ptr, i);

	return TRUE;
}

int create_torus() {
	int i, j, k, index = 0;
	torus_node_num = torus_p.p_x * torus_p.p_y * torus_p.p_z;

	torus_node_list = (torus_node *) malloc(
			sizeof(torus_node) * torus_node_num);
	if (!torus_node_list) {
		printf("malloc torus node list failed.\n");
		return FALSE;
	}

	for (i = 0; i < torus_p.p_x; ++i) {
		for (j = 0; j < torus_p.p_y; ++j) {
			for (k = 0; k < torus_p.p_z; ++k) {
				torus_node *new_node = (torus_node *) malloc(
						sizeof(torus_node));
				new_node = &torus_node_list[index];

				init_torus_node(new_node);
				if (assign_node_ip(new_node) == FALSE) {
					return FALSE;
				}
				set_coordinate(new_node, i, j, k);

				index++;
			}
		}
	}

	index = 0;
	for (i = 0; i < torus_p.p_x; ++i) {
		for (j = 0; j < torus_p.p_y; ++j) {
			for (k = 0; k < torus_p.p_z; ++k) {
				set_neighbors(&torus_node_list[index], i, j, k);
				index++;
			}
		}
	}

	return TRUE;
}

void print_torus() {
	int i;
	for (i = 0; i < torus_node_num; ++i) {
		print_torus_node(torus_node_list[i]);
	}
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
	memcpy(msg.data, &nodes_num, sizeof(int));
	for (i = 0; i < nodes_num; i++) {
		memcpy(msg.data + sizeof(int) + sizeof(struct node_info) * i,
				(void *) &nodes[i], sizeof(struct node_info));
	}

	int ret;
	if (TRUE == send_message(socketfd, msg)) {
		if (TRUE == receive_reply(socketfd)) {
			printf("%s: send torus nodes ...... finish.\n", dst_ip);
			ret = TRUE;
		} else {
			printf("%s: can't resolve message content.\n", dst_ip);
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
	int i, j, index;
	if (torus_node_list == NULL) {
		printf("update_torus: torus_node_list is null.\n");
		return FALSE;
	}

	for (i = 0; i < torus_node_num; ++i) {

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

int main(int argc, char **argv) {
	if (argc < 4) {
		printf("usage: %s x y z\n", argv[0]);
		exit(1);
	}

	// set 3-dimension partitions
	if (set_partitions(atoi(argv[1]), atoi(argv[2]), atoi(argv[3])) == FALSE) {
		exit(1);
	}

	// create a new torus
	if (create_torus() == FALSE) {
		// TODO free torus when create failed
		// free_torus
		;
	}

	print_torus();

	update_torus();

	return 0;
}

