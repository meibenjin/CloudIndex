/*
 * query_split.c
 *
 *  Created on: July 11, 2014
 *      Author: meibenjin
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>

#include "query_split.h"

//leaders_info leaders[MAX_NODES_NUM];
int cluster_id;

struct query_struct query[200000];
int query_num;

int read_query() {
    // read range query file
    char range_file[MAX_FILE_NAME];
    snprintf(range_file, MAX_FILE_NAME, "%s/range_query", DATA_DIR);
    FILE *fp = fopen(range_file, "rb");
	if (fp == NULL) {
		printf("can't open file\n");
        return FALSE;
	}

    // data format in data file
    // op data_id x_low x_high y_low y_high t_low t_high 
    int i;
    printf("begin read query.\n");
	while (!feof(fp)) {
        fscanf(fp, "%d %d", &query[query_num].op, &query[query_num].data_id);
        for (i = 0; i < MAX_DIM_NUM; i++) {
            #ifdef INT_DATA
                fscanf(fp, "%d", &query[query_num].intval[i].low);
            #else
                double low, high;
                fscanf(fp, "%lf %lf", &low, &high);
                query[query_num].intval[i].low = low;
                query[query_num].intval[i].high = high;
            #endif
        }
        fscanf(fp, "\n");
        query_num++;
    }
    printf("finish read query.\n");
	fclose(fp);
    return TRUE;
}

int split_query(int split_num) {
    int i;
    FILE *fp[split_num];
    char data_file[MAX_FILE_NAME];
    for(i = 0;i < split_num; i++) {
        snprintf(data_file, MAX_FILE_NAME, "%s/range_query_%d", DATA_DIR, i);
        fp[i] = fopen(data_file, "wb");
    }

    int idx;
    for(i = 0; i < query_num; i++) {
        idx = i % split_num;
        fprintf(fp[idx], "%d %d %lf %lf %lf %lf %lf %lf\n", 
                query[i].op, query[i].data_id, 
                query[i].intval[0].low, 
                query[i].intval[0].high, 
                query[i].intval[1].low, 
                query[i].intval[1].high, 
                query[i].intval[2].low, 
                query[i].intval[2].high);
    }

    for(i = 0;i < split_num; i++) {
        fclose(fp[i]);
    }
    return TRUE;
}

int main(int argc, char const* argv[]) {
    if (argc < 2) {
        printf("usage: %s split_num\n", argv[0]);
        exit(1);
    }

    if(FALSE == read_query()) {
        exit(1);
    }

    int split_num;
    split_num = atoi(argv[1]);

    split_query(split_num);
    return 0;
}

