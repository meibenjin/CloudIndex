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


int update_properties(struct global_properties_struct global_props){

    DEFAULT_CAPACITY=global_props.default_capacity;
    HEARTBEAT_INTERVAL=global_props.heartbeat_interval;
    MAX_ROUTE_STEP=global_props.max_route_step;
    SIGMA=global_props.sigma;
    SAMPLE_TIME_POINTS=global_props.sample_time_points;
    SAMPLE_SPATIAL_POINTS=global_props.sample_spatial_points;
    SAMPLE_TIME_RATE=global_props.sample_time_rate;
    MAX_RESPONSE_TIME=global_props.max_response_time;
    TOLERABLE_RESPONSE_TIME=global_props.tolerable_response_time;
    EXCHANGE_RATE_RANGE_QUERY=global_props.exchange_rate_range_query;
    EXCHANGE_RATE_NN_QUERY=global_props.exchange_rate_nn_query;
    EXCHANGE_RATE_PACKAGE_DATA=global_props.exchange_rate_package_data;
    ACTIVE_LEADER_NUM=global_props.active_leader_num;
    FIXED_IDLE_NODE_NUM=global_props.fixed_idle_node_num;

    return TRUE;

}

int read_properties(struct global_properties_struct *props) {
    FILE *fp;
    fp = fopen(PROPERTIES_FILE, "rb");
    if(fp == NULL) {
        printf("read_properties: open file %s failed.\n", PROPERTIES_FILE);
        return FALSE;
    }

    fscanf(fp, "DEFAULT_CAPACITY=%u\n", &props->default_capacity);
    fscanf(fp, "HEARTBEAT_INTERVAL=%d\n", &props->heartbeat_interval);
    fscanf(fp, "MAX_ROUTE_STEP=%d\n", &props->max_route_step);
    fscanf(fp, "SIGMA=%lf\n", &props->sigma);
    fscanf(fp, "SAMPLE_TIME_POINTS=%d\n", &props->sample_time_points);
    fscanf(fp, "SAMPLE_SPATIAL_POINTS=%d\n", &props->sample_spatial_points);
    fscanf(fp, "SAMPLE_TIME_RATE=%lf\n", &props->sample_time_rate);
    fscanf(fp, "MAX_RESPONSE_TIME=%d\n", &props->max_response_time);
    fscanf(fp, "TOLERABLE_RESPONSE_TIME=%d\n", &props->tolerable_response_time);
    fscanf(fp, "EXCHANGE_RATE_RANGE_QUERY=%lf\n", &props->exchange_rate_range_query);
    fscanf(fp, "EXCHANGE_RATE_NN_QUERY=%lf\n", &props->exchange_rate_nn_query);
    fscanf(fp, "EXCHANGE_RATE_PACKAGE_DATA=%lf\n", &props->exchange_rate_package_data);
    fscanf(fp, "ACTIVE_LEADER_NUM=%d\n", &props->active_leader_num);
    fscanf(fp, "FIXED_IDLE_NODE_NUM=%d\n", &props->fixed_idle_node_num);
    fclose(fp);

    return TRUE;
}

