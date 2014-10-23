/*
 * generator.h
 *
 *  Created on: June 30, 2014
 *      Author: meibenjin
 */

#ifndef GENERATOR_H_
#define GENERATOR_H_

#include "utils.h"

int gen_query(int cluster_id, coordinate c, torus_partitions tp, query_struct *query);

int read_data();

int insert_data(char entry_ips[][IP_ADDR_LENGTH], int entries_num, int cluster_id); 

#endif /* GENERATOR_H_ */
