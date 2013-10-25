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

__asm__(".symver memcpy,memcpy@GLIBC_2.2.5");

// torus node list
torus_node *torus_node_list;

// partitions on 3-dimension
torus_partitions torus_p;

// skip list (linked list for torus cluster)
skip_list *list;

char torus_ip_list[MAX_NODES_NUM][IP_ADDR_LENGTH];
int torus_nodes_num;

int set_partitions(int p_x, int p_y, int p_z) {
	if ((p_x < 1) || (p_y < 1) || (p_z < 1)) {
		printf("the number of partitions too small.\n");
		return FALSE;
	}
	if ((p_x > 20) || (p_y > 20) || (p_z > 20)) {
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
		struct coordinate c = get_node_id(node_list[i].info);
		switch (direction) {
		case X_R:
		case X_L:
			set_node_id(&node_list[i].info, t_p.p_x + c.x, c.y, c.z);
			break;
		case Y_R:
		case Y_L:
			set_node_id(&node_list[i].info, c.x, t_p.p_y + c.y, c.z);
			break;
		case Z_R:
		case Z_L:
			set_node_id(&node_list[i].info, c.x, c.y, t_p.p_z + c.z);
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

int assign_cluster_id(torus_node *node_ptr) {
	if (!node_ptr) {
		printf("assign_cluster_id: node_ptr is null pointer.\n");
		return FALSE;
	}

	static int index = 0;
	set_cluster_id(&node_ptr->info, index++);
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
				set_node_id(&new_node->info, i, j, k);
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
		c = get_node_id(torus_node_list[i].info);
		index = get_node_index(c.x, c.y, c.z);
		new_list[index] = torus_node_list[i];
	}

	for (i = 0; i < append_num; ++i) {
		c = get_node_id(append_list[i].info);
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

int send_nodes_info(OP op, const char *dst_ip, int nodes_num, struct node_info *nodes) {
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
		node_info nodes[neighbors_num + 1];

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
		get_node_ip(node_ptr->info, dst_ip);

		// send to dst_ip
		if (FALSE == send_nodes_info(UPDATE_TORUS, dst_ip, neighbors_num + 1, nodes)) {
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
	msg.op = TRAVERSE_TORUS;
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

int create_skip_list() {
	list = new_skip_list(MAXLEVEL);
	if (NULL == list) {
		return FALSE;
	}

	int i, nodes_num;

	nodes_num = get_nodes_num(torus_p);
	for (i = 0; i < nodes_num; ++i) {
        if(i == 4) {
            continue;
        }
		/*if (FALSE == insert_skip_list(list, &torus_node_list[i].info)) {
			// TODO do something when insert failed
			free(list);
			list = NULL;
			return FALSE;
		} */
        insert_skip_list_node(&torus_node_list[i].info);
	}
	return TRUE;
}

int update_skip_list() {

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

int traverse_skip_list(skip_list *slist, const char *entry_ip) {
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

int insert_skip_list_node(node_info *node_ptr) {
    int i, index, new_level, update_num;
	char src_ip[IP_ADDR_LENGTH], dst_ip[IP_ADDR_LENGTH];
    skip_list_node *sln_ptr, *new_sln;

    node_info *update_nodes, null_node, update_forward, update_backward;
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
    if(TRUE == forward_message(msg)) {
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
                && (-1 == compare(&sln_ptr->level[i].forward->leader, node_ptr))) {
            sln_ptr = sln_ptr->level[i].forward;
        }

        if (i <= new_level) {
            // update new skip list node's forward field
            new_sln->level[i].forward = sln_ptr->level[i].forward;
            update_nodes[index++] = (sln_ptr->level[i].forward == NULL) ? null_node : sln_ptr->level[i].forward->leader;

            // update new skip list node's backwaard field
			new_sln->level[i].backward = (sln_ptr == list->header) ? NULL : sln_ptr;
            update_nodes[index++] = (sln_ptr == list->header) ? null_node : sln_ptr->leader;

            // if current skip list node's forward field
            // update it's(sln_ptr->level[i].forward) backward field 
            if (sln_ptr->level[i].forward) {
                new_sln->level[i].forward->level[i].backward = new_sln;

                msg.op = UPDATE_BACKWARD;
                get_node_ip(sln_ptr->level[i].forward->leader, dst_ip);
                strncpy(msg.dst_ip, dst_ip, IP_ADDR_LENGTH);
                memcpy(msg.data, &i, sizeof(int));
                memcpy(msg.data + sizeof(int), (void *) node_ptr, sizeof(struct node_info));
                if(TRUE == forward_message(msg)) {
                    printf("update skip list node %s's backward ... success\n", dst_ip);
                } else {
                    printf("update skip list node %s's backward ... failed\n", dst_ip);
                    return FALSE;
                }
            }

            // update current skip list node's forward field 
            sln_ptr->level[i].forward = new_sln;
            if(get_cluster_id(sln_ptr->leader) != -1) {

                msg.op = UPDATE_FORWARD;
                get_node_ip(sln_ptr->leader, dst_ip);
                strncpy(msg.dst_ip, dst_ip, IP_ADDR_LENGTH);
                memcpy(msg.data, &i, sizeof(int));
                memcpy(msg.data + sizeof(int), (void *) node_ptr, sizeof(struct node_info));
                if(TRUE == forward_message(msg)) {
                    printf("update skip list node %s's forward ... success\n", dst_ip);
                } else {
                    printf("update skip list node %s's forward ... failed\n", dst_ip);
                    return FALSE;
                }
            }
        }
    }

    if(TRUE == send_nodes_info(UPDATE_SKIP_LIST, node_ptr->ip, update_num, update_nodes)) {
        return FALSE;
    }

    return TRUE;
}

int main(int argc, char **argv) {
	if (argc < 4) {
		printf("usage: %s x y z\n", argv[0]);
		exit(1);
	}

	if (FALSE == read_torus_ip_list()) {
		exit(1);
	}

	// set 3-dimension partitions
	if (set_partitions(atoi(argv[1]), atoi(argv[2]), atoi(argv[3])) == FALSE) {
		exit(1);
	}

	if (FALSE == construct_torus()) {
		exit(1);
	}
	print_torus();

	/*int direction = X_L;
	 if( FALSE == append_torus(direction)) {
	 // re-construct the original torus when append torus failed
	 construct_torus();
	 }*/

	if (TRUE == update_torus()) {
        char entry_ip[IP_ADDR_LENGTH];
        printf("input entry ip:");
        scanf("%s", entry_ip); 
        traverse_torus(entry_ip);
        if (TRUE == create_skip_list()) {
            print_skip_list(list);
            /*if (TRUE == update_skip_list()) {
                printf("update skip list success!\n");
            }*/
            traverse_skip_list(list, entry_ip);
            printf("traverse skip list success!\n");
        }
        insert_skip_list_node(&torus_node_list[4].info);
        print_skip_list(list);
	}
	return 0;
}

