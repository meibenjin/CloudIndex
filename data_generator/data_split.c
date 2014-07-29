/*
 * data_split.c
 *
 *  Created on: July 11, 2014
 *      Author: meibenjin
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>

#include "data_split.h"
#include "config/config.h"
#include "torus_node/torus_node.h"

//leaders_info leaders[MAX_NODES_NUM];
int cluster_id;

struct query_struct query[6000000];
int query_num;

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

int split_data(int split_num) {
    int i;
    FILE *fp[split_num];
    char data_file[MAX_FILE_NAME];
    for(i = 0;i < split_num; i++) {
        snprintf(data_file, MAX_FILE_NAME, "%s/data_%d", DATA_DIR, i);
        fp[i] = fopen(data_file, "wb");
    }

    int idx;
    int id = 0;
    //get torus partition
    torus_partitions tp = cluster_partitions[cluster_id];
    int num = (tp.p_x * tp.p_y * tp.p_z);
    for(i = 0; i < query_num; i++) {
        idx = query[i].trajectory_id % split_num;
        id = (query[i].data_id - 1)* num; 
        fprintf(fp[idx], "%d\t%d\t%lf\t%lf\t%lf\t%d\n", 
                query[i].op, id, 
                query[i].intval[0].low, 
                query[i].intval[1].low, 
                query[i].intval[2].low, 
                query[i].trajectory_id);
    }

    for(i = 0;i < split_num; i++) {
        fclose(fp[i]);
    }
    return TRUE;
}

int main(int argc, char const* argv[]) {
    if (argc < 2) {
        printf("usage: %s cluster_id split_num\n", argv[0]);
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

    /*for(i = 0; i < cluster_num; i++) {
        leaders[i].partition = cluster_partitions[i];
    }*/

    /*if(FALSE == read_torus_leaders(leaders, &nodes_num)) {
        exit(1);
    }*/

    if(FALSE == read_data()) {
        exit(1);
    }

    int split_num;
    cluster_id = atoi(argv[1]);
    split_num = atoi(argv[2]);

    split_data(split_num);
    return 0;
}



