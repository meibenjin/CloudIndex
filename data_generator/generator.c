/*
 * generator.c
 *
 *  Created on: June 30, 2014
 *      Author: meibenjin
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "generator.h"
#include"socket/socket.h"
#include "config/config.h"
#include "torus_node/torus_node.h"

leaders_info leaders[MAX_NODES_NUM];
int cluster_id;

struct query_struct query[15000000];
int query_num = 0;

int gen_query(int cluster_id, coordinate c, torus_partitions tp, query_struct *query) {
    int i;

    // calc each dimension's data range
    double range[MAX_DIM_NUM];
    for(i = 0; i < MAX_DIM_NUM; i++) {
        range[i] = data_region[i].high - data_region[i].low;
    }

    // translate time region
    interval time_region;
    time_region = data_region[2];
    time_region.low += range[2] * cluster_id;
    time_region.high += range[2] * cluster_id;
    query->intval[2].low += range[2] * cluster_id;
    query->intval[2].high += range[2] * cluster_id;

    double dim_intval;
    // assine current torus node's data region
    dim_intval = range[0] / tp.p_x;
    while(query->intval[0].low - dim_intval > data_region[0].low) {
        query->intval[0].low -= dim_intval;
        query->intval[0].high -= dim_intval;
    }
    query->intval[0].low += dim_intval * c.x;
    query->intval[0].high = query->intval[0].low;

    dim_intval = range[1] / tp.p_y;
    while(query->intval[1].low - dim_intval > data_region[1].low) {
        query->intval[1].low -= dim_intval;
        query->intval[1].high -= dim_intval;
    }
    query->intval[1].low += dim_intval * c.y;
    query->intval[1].high = query->intval[1].low;

    dim_intval = range[2] / tp.p_z;
    while(query->intval[2].low - dim_intval > time_region.low) {
        query->intval[2].low -= dim_intval;
        query->intval[2].high -= dim_intval;
    }
    query->intval[2].low += dim_intval * c.z + 1;
    query->intval[2].high = query->intval[2].low;

    return TRUE;
}


//close a group of sockets
//TODO this function may be moved into a file like socket_utils.c
void close_sockets(int socketfds[], int num){
    int i;
    for (i = 0;i < num;i++){
        close(socketfds[i]);
    }
}


//create a group of sockets for each ip in the  entry_ips. The obtained socketfds are saved in socketfds.
//return TRUE if all sockets are established successfully; otherwise return FALSE.
//TODO this function may be moved into a file like socket_utils.c
int create_sockets(char entry_ips[][IP_ADDR_LENGTH], int socketfds[], int entries_num){
    int i, rlt = TRUE;
    for (i = 0;i < entries_num;i++){
        socketfds[i] = new_client_socket(entry_ips[i], MANUAL_WORKER_PORT);
        if (FALSE == socketfds[i]) {
            printf("creat new socket failed for ip=%s\n", entry_ips[i]);
            rlt = FALSE;
            break;
        } 
    }
    if (FALSE == rlt){
        close_sockets(socketfds, i);
    }
     
    return rlt;
}

//package a message which is to send a query
//TODO this function may be moved into a file like message_utils.c
void package_query_msg(struct message * ptr_msg, char src_ip[IP_ADDR_LENGTH], const char dst_ip[IP_ADDR_LENGTH], int hops, struct query_struct qry){
    size_t cpy_len = 0; 
    ptr_msg->op = QUERY_TORUS_CLUSTER;
    strncpy(ptr_msg->src_ip, src_ip, IP_ADDR_LENGTH);
    strncpy(ptr_msg->dst_ip, dst_ip, IP_ADDR_LENGTH);
    strncpy(ptr_msg->stamp, "", STAMP_SIZE);

    cpy_len = 0;
    memset(ptr_msg->data, 0, DATA_SIZE);
    memcpy(ptr_msg->data, &hops, sizeof(int));
    cpy_len += sizeof(int);
    memcpy(ptr_msg->data + cpy_len, (void *)&qry, sizeof(struct query_struct));
    cpy_len += sizeof(struct query_struct);

    ptr_msg->msg_size = calc_msg_header_size() + cpy_len;
}

int insert_data_test(const char dst_ip[IP_ADDR_LENGTH]) {
    int socketfd = new_client_socket(dst_ip, MANUAL_WORKER_PORT);
    if (FALSE == socketfd) {
        printf("creat new socket failed for ip=%s\n", dst_ip);
        return FALSE;
    } 

    // get local ip
    char local_ip[IP_ADDR_LENGTH];
    memset(local_ip, 0, IP_ADDR_LENGTH);
    if (FALSE == get_local_ip(local_ip)) {
        close(socketfd);
        printf("insert_data stops because we cannot get local ip.\n");
        return FALSE;
    }

    struct message msg;
    int hops= 0;
    int query_idx = 0;
    struct query_struct new_query;

    while(query_idx<query_num) {
        new_query = query[query_idx];
        package_query_msg(&msg, local_ip, dst_ip, hops, new_query);
   
        //printf("send %d to %s\n", msg.msg_size, destination_ip);
        send_safe(socketfd, (void *) &msg, msg.msg_size, 0);
        query_idx++;
        if(query_idx % 10000 == 0) {
            printf("%d\n", query_idx);
        }
    }
    return TRUE;

}

int insert_data_origin(char entry_ips[][IP_ADDR_LENGTH], int entries_num, int cluster_id) {
    int socketfds[entries_num];
    if (FALSE == create_sockets(entry_ips, socketfds, entries_num)) {
        printf("insert_data stops because some sockets cannot be established.\n");
        return FALSE;
    }

    // get local ip
    char local_ip[IP_ADDR_LENGTH];
    memset(local_ip, 0, IP_ADDR_LENGTH);
    if (FALSE == get_local_ip(local_ip)) {
        close_sockets(socketfds, entries_num);
        printf("insert_data stops because we cannot get local ip.\n");
        return FALSE;
    }

    struct message msg;
    int hops= 0;
    struct query_struct new_query;
    char destination_ip[IP_ADDR_LENGTH];

    int query_idx = 0;
    while(query_idx<query_num) {
        new_query = query[query_idx];
        //int chosen_idx = ((int)new_query.data_id) % entries_num;
        int chosen_idx = rand() % entries_num;
        memset(destination_ip, 0, IP_ADDR_LENGTH);
        strncpy(destination_ip, entry_ips[chosen_idx], IP_ADDR_LENGTH);
        package_query_msg(&msg, local_ip, destination_ip, hops, new_query);
        //printf("send %d to %s\n", msg.msg_size, destination_ip);
        send_safe(socketfds[chosen_idx], (void *) &msg, msg.msg_size, 0);
        if(query_idx % 10000 == 0) {
            printf("%d\n", query_idx);
        }
        query_idx++;
    }

    close_sockets(socketfds, entries_num);
    return TRUE;
}
 
int insert_data(char entry_ips[][IP_ADDR_LENGTH], int entries_num, int cluster_id) {
    
    int socketfds[entries_num];
    if (FALSE == create_sockets(entry_ips, socketfds, entries_num)) {
        printf("insert_data stops because some sockets cannot be established.\n");
        return FALSE;
    }

    // get local ip
    char local_ip[IP_ADDR_LENGTH];
    memset(local_ip, 0, IP_ADDR_LENGTH);
    if (FALSE == get_local_ip(local_ip)) {
        close_sockets(socketfds, entries_num);
        printf("insert_data stops because we cannot get local ip.\n");
        return FALSE;
    }

    struct message msg;
    int hops= 0;
    struct query_struct new_query;
    int point_id;
    coordinate node_id;
    char destination_ip[IP_ADDR_LENGTH];

    //get torus partition
    torus_partitions tp = cluster_partitions[cluster_id];
    
    int i, j, k;
    int query_idx = 0;
    point_id = 0;
    int id = 0;
    while(query_idx<query_num) {
        // for each data, generate several data that match the partitions
        int traj_idx = 0;
        for(i = 0; i < tp.p_x; i++) {
            for(j = 0; j < tp.p_y; j++) {
                for(k = 0; k < tp.p_z; k++) {
                    new_query = query[query_idx];
                    new_query.data_id = id;
                    new_query.trajectory_id += traj_idx * 1000000;

                    node_id.x = i; node_id.y = j; node_id.z = k;
                    gen_query(cluster_id, node_id, tp, &new_query); 
                    
                    //int chosen_idx = ((int)new_query.data_id) % entries_num;
                    int chosen_idx = rand() % entries_num;
                    memset(destination_ip, 0, IP_ADDR_LENGTH);
                    strncpy(destination_ip, entry_ips[chosen_idx], IP_ADDR_LENGTH);

                    package_query_msg(&msg, local_ip, destination_ip, hops, new_query);
               
                    //printf("send %d to %s\n", msg.msg_size, destination_ip);
                    send_safe(socketfds[chosen_idx], (void *) &msg, msg.msg_size, 0);
                    
                    traj_idx++;
                    point_id++;
                    id++;

                    if(point_id % 10000 == 0) {
                        printf("%d\n", point_id);
                    }
                }
            }
        }
        query_idx++;
    }

    close_sockets(socketfds, entries_num);
    
    return TRUE;
}

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

int main(int argc, char const* argv[]) {
    //TODO the usage needs to be changed
    if (argc < 2) {
        printf("usage: %s cluster_id ip\n", argv[0]); 
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

    cluster_id = atoi(argv[1]);

    if(FALSE == read_data()) {
        exit(1);
    }

    if(FALSE == read_torus_ip_list()) {
        exit(1);
    }

    char entry_ips[LEADER_NUM][IP_ADDR_LENGTH];
    int i = 0;
    for(i = 0; i < LEADER_NUM; i++) {
        strncpy(entry_ips[i], torus_ip_list[i], IP_ADDR_LENGTH);
    }
    
    //insert_data(entry_ips, LEADER_NUM, cluster_id);
    insert_data_origin(entry_ips, LEADER_NUM, cluster_id);
    //insert_data_test(argv[2]);
    return 0;
}



