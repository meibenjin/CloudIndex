/*
 * config.h
 *
 *  Created on: June 27, 2014
 *      Author: meibenjin
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#include"utils.h"

// read data region file
int read_data_region();

// read ip list file
int read_torus_ip_list();

/* read cluster info file
 * each line record the torus partition
 * eg. 2 2 2
 *     3 3 3
 *     1 1 1
 * */
int read_cluster_partitions();

int read_properties(struct global_properties_struct *props);

int update_properties(struct global_properties_struct global_props);
// global config 

extern char torus_ip_list[MAX_NODES_NUM][IP_ADDR_LENGTH];
extern int torus_nodes_num;

//data region
extern interval data_region[MAX_DIM_NUM];

extern torus_partitions cluster_partitions[MAX_CLUSTERS_NUM];
extern int cluster_num;

#endif /* CONFIG_H_*/
