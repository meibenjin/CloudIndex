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

int insert_points(const char *entry_ip, int cluster_id, torus_partitions tp);

#endif /* GENERATOR_H_ */
