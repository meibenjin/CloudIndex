/*
 * config.h
 *
 *  Created on: June 27, 2014
 *      Author: meibenjin
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#include<stddef.h>
#include"utils.h"

typedef struct configuration {
	torus_partitions partitions[MAX_CLUSTERS_NUM];
	size_t cluster_num;

	char nodes[MAX_NODES_NUM][IP_ADDR_LENGTH];
	size_t nodes_num;

	interval data_region[MAX_DIM_NUM];
	struct global_properties_struct props;

} configuration, *configuration_t;

// create a new configuration instance
configuration* new_configuration();

int load_configuration(configuration_t config);

// load basic properties file
int load_properties(configuration_t config);

// load spatiotemporal data region file
int load_data_region(configuration_t config);

// load physical nodes ips
int load_nodes_list(configuration_t config);

/* each torus has a partition like 2*2*2、 3*3*3.
 * this function load the whole torus cluster's partitons
 * e.g.
 * file format:
 * 1 1 1
 * 2 2 2
 * 3 3 3
 */
int load_torus_partitions(configuration_t config);

// read data region file
//int read_data_region();

// read ip list file
//int read_torus_ip_list();

/* read cluster info file
 * each line record the torus partition
 * eg. 2 2 2
 *     3 3 3
 *     1 1 1
 * */
//int read_cluster_partitions();

int read_properties(struct global_properties_struct *props);

int update_properties(struct global_properties_struct global_props);



extern configuration_t the_config;

#endif /* CONFIG_H_*/
