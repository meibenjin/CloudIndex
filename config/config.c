/*
 * config.c
 *
 *  Created on: June 27, 2014
 *      Author: meibenjin
 */

#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include"config.h"

char torus_ip_list[MAX_NODES_NUM][IP_ADDR_LENGTH];
int torus_nodes_num;

torus_partitions cluster_partitions[MAX_CLUSTERS_NUM];
int cluster_num;

//data region
interval data_region[MAX_DIM_NUM];

int read_data_region() {
	FILE *fp;
	char region_file[MAX_FILE_NAME];
    snprintf(region_file, MAX_FILE_NAME, "%s/%s", DATA_DIR, DATA_REGION);
    fp = fopen(region_file, "rb");
    if(fp == NULL) {
		printf("read_data_region: open file %s failed.\n", region_file);
		return FALSE;
    }

    int i;
    for (i = 0; i < MAX_DIM_NUM; i++) {
        #ifdef INT_DATA
            fscanf(fp, "%d\t%d", &data_region[i].low, &data_region[i].high);
        #else
            fscanf(fp, "%lf\t%lf", &data_region[i].low, &data_region[i].high);
        #endif
    }
    fclose(fp);
    return TRUE;
}

int read_torus_ip_list() {
	FILE *fp;
	char ip[IP_ADDR_LENGTH];
	int count;
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
    fclose(fp);
	return TRUE;
}

int read_cluster_partitions() {
    FILE *fp;
    int count;
    fp = fopen(CLUSTER_PARTITONS, "rb");
    if(fp == NULL) {
        printf("read_cluster_partitons: open file %s failed.\n", CLUSTER_PARTITONS);
        return FALSE;
    }

    count = 0;
    torus_partitions tp;
	while (!feof(fp)) {
        fscanf(fp, "%d %d %d\n", &tp.p_x, &tp.p_y, &tp.p_z);
        cluster_partitions[count] = tp;
        count++;
    }
    cluster_num = count;
    fclose(fp);
    return TRUE;
}




