/*
 * data_partition.c
 *
 *  Created on: July 30, 2014
 *      Author: meibenjin
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include<pthread.h>

#include "config/config.h"
#include "torus_node/torus_node.h"

leaders_info leaders[MAX_NODES_NUM];
int cluster_id;

struct query_struct query[6000000];
int query_num = 0;

//TODO this function is exactly the same as the gen_query function in generator.c
// we may move this function into load_data.c
// the name is better changed to transform_query
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

int write_to_file(struct query_struct query, coordinate file_id) {
    int x, y, z;
    x = file_id.x;
    y = file_id.y;
    z = file_id.z;
    char file_name[MAX_FILE_NAME];
    sprintf(file_name, "%s/data_%d%d%d", DATA_DIR, x, y, z);
    FILE *fp = fopen(file_name, "ab+");
    if(fp == NULL) {
        printf("can't open file %s.\n", file_name);
        exit(0);
    }

    return TRUE;
}

int data_partition(int cluster_id) {
    int i, j, k;
    int query_idx = 0;
    int id = 0;

    struct query_struct new_query;
    coordinate node_id;

    //get torus partition
    torus_partitions tp = cluster_partitions[cluster_id];
    int torus_num = tp.p_x * tp.p_y * tp.p_z;

    FILE *fp[torus_num];
    for(i = 0; i < torus_num; i++) {
        char file_name[MAX_FILE_NAME];
        sprintf(file_name, "%s/data_%d", DATA_DIR, i);
        fp[i] = fopen(file_name, "wb+");
        if(fp[i] == NULL) {
            printf("can't open file %s.\n", file_name);
            exit(0);
        }
    }

    while(query_idx<query_num) {
        // for each data, generate several data that match the partitions
        int traj_idx = 0;
        int file_id = 0;
        for(i = 0; i < tp.p_x; i++) {
            for(j = 0; j < tp.p_y; j++) {
                for(k = 0; k < tp.p_z; k++) {
                    new_query = query[query_idx];
                    new_query.data_id = id;
                    new_query.trajectory_id += traj_idx * 1000000;

                    node_id.x = i; node_id.y = j; node_id.z = k;
                    gen_query(cluster_id, node_id, tp, &new_query); 

                    fprintf(fp[file_id], "%d\t%d\t", new_query.op, new_query.data_id);
                    int l;
                    for (l = 0; l < MAX_DIM_NUM; l++) {
                        double value = new_query.intval[l].low;
                        fprintf(fp[file_id], "%lf\t", value);
                    }
                    fprintf(fp[file_id], "%d", new_query.trajectory_id);
                    fprintf(fp[file_id], "\n");

                    // package query into message struct
                    traj_idx++;
                    file_id++;
                    id++;
                }
            }
        }
        query_idx++;
    }

    for(i = 0; i < torus_num; i++) {
        fclose(fp[i]);
    }
    return TRUE;
}

//TODO this function is exactly the same as the read_data function in generator.c
// we may move this function into load_data.c
int read_data() {
    // read data file
    char data_file[MAX_FILE_NAME];
    snprintf(data_file, MAX_FILE_NAME, "%s/data", DATA_DIR);
    FILE *fp = fopen(data_file, "rb");
	if (fp == NULL) {
		printf("can't open file\n");
        return FALSE;
	}

    // data format in data file
    // op data_id lat lon time obj_id(traj_id)
    int i;
    printf("begin read data.\n");
	while (!feof(fp)) {
        fscanf(fp, "%d\t%d\t", &query[query_num].op, &query[query_num].data_id);
        for (i = 0; i < MAX_DIM_NUM; i++) {
            #ifdef INT_DATA
                fscanf(fp, "%d", &query[query_num].intval[i].low);
            #else
                double value;
                fscanf(fp, "%lf", &value);
                query[query_num].intval[i].low = value;
                query[query_num].intval[i].high = value;
            #endif
        }
        fscanf(fp, "%d", &query[query_num].trajectory_id);
        fscanf(fp, "\n");
        query_num++;
    }
    printf("finish read data.\n");
	fclose(fp);
    return TRUE;
}

int main(int argc, char const* argv[]) {
    if (argc < 1) {
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

    cluster_id = atoi(argv[1]);

    printf("cluster:%d\n", cluster_id); 

    if(FALSE == read_data()) {
        exit(1);
    }

    data_partition(cluster_id);

    return 0;
}

