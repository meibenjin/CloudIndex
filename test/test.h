/*
 * test.h
 *
 *  Created on: Jan 22, 2015
 *      Author: meibenjin
 */

#ifndef TEST_H_
#define TEST_H_

int check_traj_seq(int cluster_id);

int check_system_status(int cluster_id);

int check_conn_buffer(int cluster_id);

int check_insert_data(int cluster_id);

int query(const char *entry_ip, char *file_name, int time_gap);

#endif
