/*
 * test.c
 *
 *  Created on: Sep 17, 2013
 *      Author: meibenjin
 */

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/time.h>
#include<time.h>
#include<unistd.h>
#include<math.h>
#include<sys/epoll.h>
#include<pthread.h>
#include<errno.h>

#include "utils.h"
#include "socket/socket.h"

int query_oct_tree(struct query_struct query, const char *entry_ip) {

	// get local ip address
	char local_ip[IP_ADDR_LENGTH];
	memset(local_ip, 0, IP_ADDR_LENGTH);
	if (FALSE == get_local_ip(local_ip)) {
		return FALSE;
	}

	struct message msg;
	msg.op = QUERY_TORUS_CLUSTER;
	strncpy(msg.src_ip, local_ip, IP_ADDR_LENGTH);
	strncpy(msg.dst_ip, entry_ip, IP_ADDR_LENGTH);
	strncpy(msg.stamp, "", STAMP_SIZE);

    size_t cpy_len = 0; 
	int count = 0;
	memcpy(msg.data, &count, sizeof(int));
    cpy_len += sizeof(int);

    memcpy(msg.data + cpy_len, (void *)&query, sizeof(struct query_struct));
    cpy_len += sizeof(struct query_struct);

    if(FALSE == forward_message(msg, 0)) {
        return FALSE;
    }

	return TRUE;
}

//return elasped time from start to finish
long get_elasped_time(struct timespec start, struct timespec end) {
	return 1000000000L * (end.tv_sec - start.tv_sec)
			+ (end.tv_nsec - start.tv_nsec);
}


int range_query(const char *entry_ip, char *file_name, int time_gap) {
	int count = 0, i;
    struct query_struct query;
    FILE *fp;

    char file[MAX_FILE_NAME];
    snprintf(file, MAX_FILE_NAME, "%s/%s", DATA_DIR, file_name);
	fp = fopen(file, "rb");
	if (fp == NULL) {
		printf("can't open file %s\n", file);
		exit(1);
	}

    // create socket to server for transmitting data points
    int socketfd;
    socketfd = new_client_socket(entry_ip, MANUAL_WORKER_PORT);
    if (FALSE == socketfd) {
        return FALSE;
    }

    // get local ip
	char local_ip[IP_ADDR_LENGTH];
	memset(local_ip, 0, IP_ADDR_LENGTH);
	if (FALSE == get_local_ip(local_ip)) {
        close(socketfd);
		return FALSE;
	}

    // basic packet to be sent
    struct message msg;
    msg.op = QUERY_TORUS_CLUSTER;
	strncpy(msg.src_ip, local_ip, IP_ADDR_LENGTH);
	strncpy(msg.dst_ip, entry_ip, IP_ADDR_LENGTH);
	strncpy(msg.stamp, "", STAMP_SIZE);

    size_t cpy_len = 0; 
	int hops= 0;

    if(strcmp(file_name, "range_query") == 0){
        printf("begin range query.\n");
        while (!feof(fp)) {
            fscanf(fp, "%d %d", &query.op, &query.data_id);
            //printf("%d. ", ++count);
            for (i = 0; i < MAX_DIM_NUM; i++) {
                fscanf(fp, "%lf %lf ", &query.intval[i].low, &query.intval[i].high);
            }
            fscanf(fp, "\n");
            /*printf("%d %d %lf %lf %lf %lf %lf %lf\n", query.op, query.data_id, \
                    query.intval[0].low, query.intval[0].high, \
                    query.intval[1].low, query.intval[1].high,\
                    query.intval[2].low, query.intval[2].high);*/

            // package query into message struct
            cpy_len = 0;
            memset(msg.data, 0, DATA_SIZE);
            memcpy(msg.data, &hops, sizeof(int));
            cpy_len += sizeof(int);
            memcpy(msg.data + cpy_len, (void *)&query, sizeof(struct query_struct));
            cpy_len += sizeof(struct query_struct);
            send_safe(socketfd, (void *) &msg, sizeof(struct message), 0);
            if(count % time_gap == 0) {
                printf("%d\n", count);
            }
            count++;
            usleep(time_gap * 1000);
        }
        printf("finish range query.\n");
        fclose(fp);
    }

    if(strcmp(file_name, "nn_query") == 0){
        printf("begin nn query.\n");
        while (!feof(fp)) {
            fscanf(fp, "%d %d", &query.op, &query.data_id);
            //printf("%d. ", ++count);
            for (i = 0; i < MAX_DIM_NUM; i++) {
                fscanf(fp, "%lf %lf ", &query.intval[i].low, &query.intval[i].high);
            }
            fscanf(fp, "\n");

            // package query into message struct
            cpy_len = 0;
            memset(msg.data, 0, DATA_SIZE);
            memcpy(msg.data, &hops, sizeof(int));
            cpy_len += sizeof(int);
            memcpy(msg.data + cpy_len, (void *)&query, sizeof(struct query_struct));
            cpy_len += sizeof(struct query_struct);
            send_safe(socketfd, (void *) &msg, sizeof(struct message), 0);
            if(count % time_gap == 0) {
                printf("%d\n", count);
            }
            count++;
            usleep(time_gap * 1000);
        }
        printf("finish nn query.\n");
        fclose(fp);
    }
    close(socketfd);

    return TRUE;
}

int main(int argc, char **argv){
    if (argc < 4) {
        printf("usage: %s leader_ip query_file time_gap\n", argv[0]);
        exit(1);
    }
    range_query(argv[1], argv[2], atoi(argv[3]));
    return 0;
}


