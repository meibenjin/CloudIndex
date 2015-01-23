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
#include"logs/log.h"

/*char torus_ip_list[MAX_NODES_NUM][IP_ADDR_LENGTH];
int torus_nodes_num;
torus_partitions cluster_partitions[MAX_CLUSTERS_NUM];
int cluster_num;*/

//data region
//interval data_region[MAX_DIM_NUM];
//
configuration_t the_config = NULL;

static void init_configuration(configuration_t config);

static void init_configuration(configuration_t config) {
	config->cluster_num = 0;
	config->nodes_num = 0;
}

configuration* new_configuration() {
	configuration *new_config;
	new_config = (configuration *) malloc(sizeof(struct configuration));
	if (new_config == NULL) {
		write_log(ERROR_LOG,
				"new_configuration: malloc for connection manager failed.\n");
		return NULL;
	}
	init_configuration(new_config);

	return new_config;
}

int load_configuration(configuration_t config) {
	if (NULL == config) {
		write_log(ERROR_LOG, "load_configuration: configuration is NULL.\n");
		return FALSE;
	}

	if (FALSE == load_nodes_list(config)) {
		return FALSE;
	}

	if (FALSE == load_data_region(config)) {
		return FALSE;
	}

	if (FALSE == load_torus_partitions(config)) {
		return FALSE;
	}

	if (FALSE == load_properties(config)) {
		return FALSE;
	}

	return TRUE;
}

int load_properties(configuration_t config) {
	if (NULL == config) {
		write_log(ERROR_LOG, "load_properties: configuration is NULL.\n");
		return FALSE;
	}

    read_properties(&config->props);

	return TRUE;
}

int load_data_region(configuration_t config) {
	if (NULL == config) {
		write_log(ERROR_LOG, "load_data_region: configuration is NULL.\n");
		return FALSE;
	}

	FILE *fp;
	char region_file[MAX_FILE_NAME];
	snprintf(region_file, MAX_FILE_NAME, "%s/%s", DATA_DIR, DATA_REGION);
	fp = fopen(region_file, "rb");
	if (fp == NULL) {
		write_log(ERROR_LOG, "load_data_region: open file %s failed.\n",
				region_file);
		return FALSE;
	}

	int i;
	for (i = 0; i < MAX_DIM_NUM; i++) {
#ifdef INT_DATA
		fscanf(fp, "%d\t%d", &config->data_region[i].low, &config->data_region[i].high);
#else
		fscanf(fp, "%lf\t%lf", &config->data_region[i].low,
				&config->data_region[i].high);
#endif
	}
	fclose(fp);
	return TRUE;
}

// load physical nodes ips
int load_nodes_list(configuration_t config) {
	if (NULL == config) {
		write_log(ERROR_LOG, "load_nodes_list: configuration is NULL.\n");
		return FALSE;
	}

	FILE *fp;
	char ip[IP_ADDR_LENGTH];
	int count;
	fp = fopen(TORUS_IP_LIST, "rb");
	if (fp == NULL) {
		write_log(ERROR_LOG, "load_nodes_list: open file %s failed.\n",
		TORUS_IP_LIST);
		return FALSE;
	}

	count = 0;
	while ((fgets(ip, IP_ADDR_LENGTH, fp)) != NULL) {
		ip[strlen(ip) - 1] = '\0';
		strncpy(config->nodes[count], ip, IP_ADDR_LENGTH);
		count++;

		if (count == MAX_NODES_NUM) {
			write_log(ERROR_LOG,
					"load_nodes_list: too many nodes in file %s.\n",
					TORUS_IP_LIST);
			fclose(fp);
			return FALSE;
		}
	}
	config->nodes_num = count;

	fclose(fp);
	return TRUE;
}

int load_torus_partitions(configuration_t config) {
	if (NULL == config) {
		write_log(ERROR_LOG, "load_torus_partitions: configuration is NULL.\n");
		return FALSE;
	}

	FILE *fp;
	int count;
	fp = fopen(CLUSTER_PARTITONS, "rb");
	if (fp == NULL) {
		write_log(ERROR_LOG, "load_torus_partitions: open file %s failed.\n",
		CLUSTER_PARTITONS);
		return FALSE;
	}

	count = 0;
	torus_partitions tp;
	while (!feof(fp)) {
		fscanf(fp, "%d %d %d\n", &tp.p_x, &tp.p_y, &tp.p_z);
		config->partitions[count] = tp;
		count++;
		if (count == MAX_CLUSTERS_NUM) {
			write_log(ERROR_LOG,
					"load_torus_partitions: too many torus partitions in file %s.\n",
					CLUSTER_PARTITONS);
			fclose(fp);
			return FALSE;
		}
	}
	config->cluster_num = count;
	fclose(fp);
	return TRUE;
}

int update_properties(struct global_properties_struct props) {

	RUNNING_MODE = props.running_mode;
	NUM_REPLICAS = props.num_replicas;
	CPU_CORE = props.cpu_core;
	DEFAULT_CAPACITY = props.default_capacity;
	HEARTBEAT_INTERVAL = props.heartbeat_interval;
	MAX_ROUTE_STEP = props.max_route_step;
	SIGMA = props.sigma;
	SAMPLE_TIME_POINTS = props.sample_time_points;
	SAMPLE_SPATIAL_POINTS = props.sample_spatial_points;
	SAMPLE_TIME_RATE = props.sample_time_rate;
	MAX_RESPONSE_TIME = props.max_response_time;
	TOLERABLE_RESPONSE_TIME = props.tolerable_response_time;
	EXCHANGE_RATE_RANGE_QUERY = props.exchange_rate_range_query;
	EXCHANGE_RATE_NN_QUERY = props.exchange_rate_nn_query;
	EXCHANGE_RATE_PACKAGE_DATA = props.exchange_rate_package_data;
	ESTIMATE_NN_QUERY_COEFFICIENT = props.estimate_nn_query_coefficient;
	ACTIVE_LEADER_NUM = props.active_leader_num;
	FIXED_IDLE_NODE_NUM = props.fixed_idle_node_num;

	return TRUE;

}

int read_properties(struct global_properties_struct *props) {
	FILE *fp;
	fp = fopen(PROPERTIES_FILE, "rb");
	if (fp == NULL) {
		printf("read_properties: open file %s failed.\n", PROPERTIES_FILE);
		return FALSE;
	}

	fscanf(fp, "RUNNING_MODE=%d\n", &props->running_mode);
	fscanf(fp, "NUM_REPLICAS=%d\n", &props->num_replicas);
	fscanf(fp, "CPU_CORE=%u\n", &props->cpu_core);
	fscanf(fp, "DEFAULT_CAPACITY=%u\n", &props->default_capacity);
	fscanf(fp, "HEARTBEAT_INTERVAL=%d\n", &props->heartbeat_interval);
	fscanf(fp, "MAX_ROUTE_STEP=%d\n", &props->max_route_step);
	fscanf(fp, "SIGMA=%lf\n", &props->sigma);
	fscanf(fp, "SAMPLE_TIME_POINTS=%d\n", &props->sample_time_points);
	fscanf(fp, "SAMPLE_SPATIAL_POINTS=%d\n", &props->sample_spatial_points);
	fscanf(fp, "SAMPLE_TIME_RATE=%lf\n", &props->sample_time_rate);
	fscanf(fp, "MAX_RESPONSE_TIME=%d\n", &props->max_response_time);
	fscanf(fp, "TOLERABLE_RESPONSE_TIME=%d\n", &props->tolerable_response_time);
	fscanf(fp, "EXCHANGE_RATE_RANGE_QUERY=%lf\n",
			&props->exchange_rate_range_query);
	fscanf(fp, "EXCHANGE_RATE_NN_QUERY=%lf\n", &props->exchange_rate_nn_query);
	fscanf(fp, "EXCHANGE_RATE_PACKAGE_DATA=%lf\n",
			&props->exchange_rate_package_data);
	fscanf(fp, "ESTIMATE_NN_QUERY_COEFFICIENT=%lf\n",
			&props->estimate_nn_query_coefficient);
	fscanf(fp, "ACTIVE_LEADER_NUM=%d\n", &props->active_leader_num);
	fscanf(fp, "FIXED_IDLE_NODE_NUM=%d\n", &props->fixed_idle_node_num);
	fclose(fp);

	return TRUE;
}


