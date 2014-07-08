/*
 * generator.c
 *
 *  Created on: June 30, 2014
 *      Author: meibenjin
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "generator.h"
#include"socket/socket.h"
#include "config/config.h"
#include "torus_node/torus_node.h"


int gen_query(int cluster_id, coordinate c, torus_partitions tp, query_struct *query) {
    int i;

    // calc each dimension's data range
    double range[MAX_DIM_NUM];
    for(i = 0; i < MAX_DIM_NUM; i++) {
        range[i] = data_region[i].high - data_region[i].low;
    }

    // translate time region
    interval time_region;
    time_region = data_region[2];
    time_region.low += range[2] * cluster_id;
    time_region.high += range[2] * cluster_id;
    query->intval[2].low += range[2] * cluster_id;
    query->intval[2].high += range[2] * cluster_id;

    double dim_intval;
    // assine current torus node's data region
    dim_intval = range[0] / tp.p_x;
    while(query->intval[0].low - dim_intval > data_region[0].low) {
        query->intval[0].low -= dim_intval;
        query->intval[0].high -= dim_intval;
    }
    query->intval[0].low += dim_intval * c.x;
    query->intval[0].high = query->intval[0].low;

    dim_intval = range[1] / tp.p_y;
    while(query->intval[1].low - dim_intval > data_region[1].low) {
        query->intval[1].low -= dim_intval;
        query->intval[1].high -= dim_intval;
    }
    query->intval[1].low += dim_intval * c.y;
    query->intval[1].high = query->intval[1].low;

    dim_intval = range[2] / tp.p_z;
    while(query->intval[2].low - dim_intval > time_region.low) {
        query->intval[2].low -= dim_intval;
        query->intval[2].high -= dim_intval;
    }
    query->intval[2].low += dim_intval * c.z + 1;
    query->intval[2].high = query->intval[2].low;

    return TRUE;
}


int insert_points(const char *entry_ip, int cluster_id, torus_partitions tp) {
    // create socket to server for transmitting data points
    int socketfd;
    socketfd = new_client_socket(entry_ip, MANUAL_WORKER_PORT);
    if (FALSE == socketfd) {
        return FALSE;
    }

    // get local ip
	char local_ip[IP_ADDR_LENGTH];
	memset(local_ip, 0, IP_ADDR_LENGTH);
	if (FALSE == get_local_ip(local_ip)) {
        close(socketfd);
		return FALSE;
	}

    // basic packet to be sent
    struct message msg;
    msg.op = QUERY_TORUS_CLUSTER;
	strncpy(msg.src_ip, local_ip, IP_ADDR_LENGTH);
	strncpy(msg.dst_ip, entry_ip, IP_ADDR_LENGTH);
	strncpy(msg.stamp, "", STAMP_SIZE);

    size_t cpy_len = 0; 
	int hops= 0;

    // read data file
    char data_file[MAX_FILE_NAME];
    snprintf(data_file, MAX_FILE_NAME, "%s/data", DATA_DIR);
    FILE *fp = fopen(data_file, "rb");
	if (fp == NULL) {
		printf("can't open file\n");
		exit(1);
	}

    // data format in data file
    // op data_id lat lon time obj_id(traj_id)
    int i, j, k, count = 0;
    struct query_struct query, new_query;
    coordinate node_id;
    printf("begin read.\n");
	while (!feof(fp)) {
        fscanf(fp, "%d\t%d\t", &query.op, &query.data_id);
        for (i = 0; i < MAX_DIM_NUM; i++) {
            #ifdef INT_DATA
                fscanf(fp, "%d", &query.intval[i].low);
            #else
                double value;
                fscanf(fp, "%lf", &value);
                query.intval[i].low = value;
                query.intval[i].high = value;
            #endif
        }
        fscanf(fp, "%d", &query.trajectory_id);
        fscanf(fp, "\n");
        //printf("%lf %lf %lf\n", query.intval[0].low, query.intval[1].low, query.intval[2].low);

        // for each data, generate more data
        for(i = 0; i < tp.p_x; i++) {
            for(j = 0; j < tp.p_y; j++) {
                for(k = 0; k < tp.p_z; k++) {
                    node_id.x = i; node_id.y = j; node_id.z = k;
                    new_query = query;
                    gen_query(cluster_id, node_id, tp, &new_query); 

                    //printf("\t%lf %lf %lf\n", new_query.intval[0].low, new_query.intval[1].low, new_query.intval[2].low);

                    // package query into message struct
                    cpy_len = 0;
                    memset(msg.data, 0, DATA_SIZE);
                    memcpy(msg.data, &hops, sizeof(int));
                    cpy_len += sizeof(int);
                    memcpy(msg.data + cpy_len, (void *)&new_query, sizeof(struct query_struct));
                    cpy_len += sizeof(struct query_struct);
                    send_safe(socketfd, (void *) &msg, sizeof(struct message), 0);

                    count++;
                }
            }
        }


        if(count % 10000 == 0) {
            printf("%d\n", count);
        }
	}
    printf("finish read.\n");
	fclose(fp);

    close(socketfd);
    return TRUE;
}

int main(int argc, char const* argv[]) {
    if (argc < 2) {
        printf("usage: %s cluster_id\n", argv[0]);
        exit(1);
    }

    // read test data region from file 
    if(FALSE == read_data_region()) {
        exit(1);
    }

    // read cluster partitions from file 
    if(FALSE == read_cluster_partitions()) {
        exit(1);
    }

    leaders_info leaders[MAX_NODES_NUM];
    int nodes_num = 0;

    int i;
    for(i = 0; i < cluster_num; i++) {
        leaders[i].partition = cluster_partitions[i];
    }

    if(FALSE == read_torus_leaders(leaders, &nodes_num)) {
        exit(1);
    }

    int cluster_id, index;
    char entry_ip[IP_ADDR_LENGTH];

    cluster_id = atoi(argv[1]);
    index = rand() % LEADER_NUM;

    strncpy(entry_ip, leaders[cluster_id].ip[index], IP_ADDR_LENGTH);
    strncpy(entry_ip, "172.16.0.212", IP_ADDR_LENGTH);
    printf("send ip: %s\n", entry_ip);

    insert_points(entry_ip, cluster_id, leaders[cluster_id].partition);

    return 0;
}



