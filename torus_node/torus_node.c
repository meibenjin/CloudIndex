/*
 * torus_node.c
 *
 *  Created on: Sep 16, 2013
 *      Author: meibenjin
 */

#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include"torus_node.h"
#include"logs/log.h"

//__asm__(".symver memcpy,memcpy@GLIBC_2.2.5");

void init_node_info(node_info *info_ptr) {
	if (!info_ptr) {
		printf("init_torus_node: node_ptr is null pointer.\n");
		return;
	}
	set_node_ip(info_ptr, "0.0.0.0");
	set_node_id(info_ptr, -1, -1, -1);
	set_cluster_id(info_ptr, -1);
    set_node_capacity(info_ptr, 0);

	int i;
	for(i = 0; i < MAX_DIM_NUM; ++i){
		info_ptr->region[i].low = 0;
		info_ptr->region[i].high = 0;
	}
}

void init_torus_node(torus_node *node_ptr) {
	if (!node_ptr) {
		printf("init_torus_node: node_ptr is null pointer.\n");
		return;
	}
    init_node_info(&node_ptr->info);
}

int set_interval(node_info *node_ptr){
    int i;
    int cid = node_ptr->cluster_id;
    coordinate nid = node_ptr->node_id;

    char range_file[MAX_FILE_NAME];
    snprintf(range_file, MAX_FILE_NAME, "%s/r%d_%d%d%d", TMP_DATA_DIR, cid, nid.x, nid.y, nid.z);
    FILE *fp = fopen(range_file, "rb");
    if(fp == NULL) {
        printf("can't open range file %s\n", range_file);
        return FALSE;
    }
    for (i = 0; i < MAX_DIM_NUM; i++) {
        #ifdef INT_DATA
            fscanf(fp, "%d %d", &node_ptr->region[i].low, &node_ptr->region[i].high);
        #else
            fscanf(fp, "%lf %lf", &node_ptr->region[i].low, &node_ptr->region[i].high);
        #endif
    }
    fclose(fp);
	return TRUE;
}

void set_node_capacity(node_info *info_ptr, int c) {
	if (!info_ptr) {
		printf("set_node_capacity: node_ptr is null pointer.\n");
		return;
	}

    info_ptr->capacity = c;
}

int get_node_capacity(node_info info) {
    return info.capacity;
}

void set_node_ip(node_info *info_ptr, const char *ip) {
	if (!info_ptr) {
		printf("set_node_ip: node_ptr is null pointer.\n");
		return;
	}
	strncpy(info_ptr->ip, ip, IP_ADDR_LENGTH);
}

void get_node_ip(node_info info, char *ip) {
	if (ip == NULL) {
		printf("get_node_ip: ip is null pointer.\n");
		return;
	}
	strncpy(ip, info.ip, IP_ADDR_LENGTH);
}

void set_node_id(node_info *info_ptr, int x, int y, int z) {
	if (!info_ptr) {
		printf("set_coordinate: node_ptr is null pointer.\n");
		return;
	}
	info_ptr->node_id.x = x;
	info_ptr->node_id.y = y;
	info_ptr->node_id.z = z;
}

struct coordinate get_node_id(node_info info) {
	return info.node_id;
}

void set_cluster_id(node_info *info_ptr, int cluster_id) {
	if (!info_ptr) {
		printf("set_cluster_id: node_ptr is null pointer.\n");
		return;
	}
	info_ptr->cluster_id = cluster_id;
}

int get_cluster_id(node_info info) {
	return info.cluster_id;
}

int add_neighbor_info(torus_node *node_ptr, int d, node_info *neighbor_ptr) {
    struct neighbor_node *nn_ptr;
    nn_ptr = (struct neighbor_node *) malloc(sizeof(struct neighbor_node));
    if(nn_ptr == NULL) {
        printf("add_neighbor_info: malloc neighbor_node failed\n");
        return FALSE;
    }
    nn_ptr->info = neighbor_ptr;
    nn_ptr->next = NULL;

    // if insert the first neighbor node at direction
    if(node_ptr->neighbors[d] == NULL) {
        node_ptr->neighbors[d] = (struct neighbor_node*) malloc(sizeof(struct neighbor_node));
        if(node_ptr->neighbors[d] == NULL) {
            printf("add_neighbor_info: malloc for neighbor node info failed.\n");
            return FALSE;
        }
        node_ptr->neighbors[d]->info = NULL;
        node_ptr->neighbors[d]->next = NULL;
    }

    nn_ptr->next = node_ptr->neighbors[d]->next;
    node_ptr->neighbors[d]->next = nn_ptr;

    return TRUE;
}

void set_neighbors_num(torus_node *node_ptr, int neighbors_num) {
	if (!node_ptr) {
		printf("set_neighbors_num: node_ptr is null pointer.\n");
		return;
	}

	node_ptr->neighbors_num = neighbors_num;
}

int get_neighbors_num_d(torus_node *node_ptr, int d) {
    int count = 0;
    if(node_ptr->neighbors[d] != NULL) {
        struct neighbor_node *nn_ptr;
        nn_ptr = node_ptr->neighbors[d]->next;
        while(nn_ptr != NULL) {
            count++;
            nn_ptr = nn_ptr->next;
        }
    }
    return count;
}

int get_neighbors_num(torus_node *node_ptr) {
	return node_ptr->neighbors_num;
}

node_info *get_neighbor_by_id(torus_node *node_ptr, struct coordinate node_id){
	int d, x, y, z;
	//int neighbors_num = get_neighbors_num(node);
	for (d = 0; d < DIRECTIONS; ++d) {
        if(node_ptr->neighbors[d] != NULL) {
            struct neighbor_node *nn_ptr;
            nn_ptr = node_ptr->neighbors[d]->next;
            while(nn_ptr != NULL) {
                x = nn_ptr->info->node_id.x;
                y = nn_ptr->info->node_id.y; 
                z = nn_ptr->info->node_id.z; 
                if ((node_id.x == x) && (node_id.y == y) && (node_id.z == z)) {
                    return nn_ptr->info;
                }
                nn_ptr = nn_ptr->next;
            }
        }
	}
    return NULL;
}

void print_neighbors(torus_node node) {
	//int neighbors_num = get_neighbors_num(node);

    printf("neighbors:\n");
    #ifdef WRITE_LOG
        write_log(TORUS_NODE_LOG, "neighbor:\n");
    #endif

	int d;
	for (d = 0; d < DIRECTIONS; ++d) {
        if(node.neighbors[d] != NULL) {
            struct neighbor_node *nn_ptr;
            nn_ptr = node.neighbors[d]->next;
            while(nn_ptr != NULL) {
                printf("\t");
                print_node_info(*nn_ptr->info);
                nn_ptr = nn_ptr->next;
            }
        }
	}
}

torus_node *new_torus_node() {
	torus_node *new_torus;
	new_torus = (torus_node *) malloc(sizeof(torus_node));
	if (new_torus == NULL) {
		printf("new_torus_node: malloc for torus node failed.\n");
		return NULL;
	}
	init_node_info(&new_torus->info);
	set_neighbors_num(new_torus, 0);

    int i;
    for(i = 0; i < DIRECTIONS; i++) {
        new_torus->neighbors[i] = NULL;
    }

	return new_torus;
}

void print_torus_node(torus_node torus) {
	print_node_info(torus.info);
	print_neighbors(torus);
	printf("\n");
}

void print_node_info(node_info node) {
	int i;
    int cid = node.cluster_id;
    coordinate nid = node.node_id;

    printf("[%d_%d%d%d]", cid, nid.x, nid.y, nid.z);
	printf("%s:", node.ip);

	for(i = 0; i < MAX_DIM_NUM; ++i){
        #ifdef INT_DATA
            printf("[%d, %d] ", node.region[i].low, node.region[i].high);
        #else 
            printf("[%lf, %lf] ", node.region[i].low, node.region[i].high);
        #endif
	}
	printf("\n");

    #ifdef WRITE_LOG 
        char buf[1024];
        memset(buf, 0, 1024);
        int len = 0;
        len = sprintf(buf, "[%d_%d%d%d]", cid, nid.x, nid.y, nid.z);
        len += sprintf(buf + len, "%s:", node.ip);
        for(i = 0; i < MAX_DIM_NUM; ++i){
            #ifdef INT_DATA
                len += sprintf(buf + len, "[%d, %d] ", node.region[i].low, node.region[i].high);
            #else 
                len += sprintf(buf + len, "[%lf, %lf] ", node.region[i].low, node.region[i].high);
            #endif
        }
        sprintf(buf+len, "\n");
        write_log(TORUS_NODE_LOG, buf);
    #endif
}

void print_torus_leaders(node_info leaders[]) {
    #ifdef WRITE_LOG 
        char buf[1024];
        memset(buf, 0, 1024);
        int len = 0, i;
        len = sprintf(buf, "torus leaders:[");
        for(i = 0; i < LEADER_NUM; i++) {
            len += sprintf(buf + len, "%s ", leaders[i].ip);
        }
        len += sprintf(buf + len, "]\n");
        write_log(TORUS_NODE_LOG, buf);
    #endif
}

