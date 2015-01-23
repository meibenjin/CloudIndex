/*
 * test_range_nn_query.c
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
#include "communication/socket.h"
#include"communication/message.h"

#include "test.h"

static int send_query(const char *entry_ip, const char *file, int time_gap) {
	int i, socketfd;
    FILE *fp;

    // basic packet to be sent
    struct message msg;
    struct query_struct query;

    // create socket to server for transmitting data points
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

	fp = fopen(file, "rb");
	if (fp == NULL) {
		printf("can't open file %s\n", file);
        return FALSE;
	}

	int hops= 0;
    size_t cpy_len = 0; 
    while (!feof(fp)) {
        fscanf(fp, "%d %d", &query.op, &query.data_id);
        for (i = 0; i < MAX_DIM_NUM; i++) {
            fscanf(fp, "%lf %lf ", &query.intval[i].low, &query.intval[i].high);
        }
        fscanf(fp, "\n");

        // package query into message struct
        cpy_len = 0;
        char buf[DATA_SIZE];
        memset(buf, 0, DATA_SIZE);
        memcpy(buf, &hops, sizeof(int));
        cpy_len += sizeof(int);
        memcpy(buf + cpy_len, (void *)&query, sizeof(struct query_struct));
        cpy_len += sizeof(struct query_struct);

        msg.msg_size = calc_msg_header_size() + cpy_len;
		fill_message(msg.msg_size, QUERY_TORUS_CLUSTER, local_ip, entry_ip, \
                "", buf, cpy_len, &msg);

        send_data(socketfd, (void *) &msg, msg.msg_size);

        printf("query %d:", query.data_id);
        for (i = 0; i < MAX_DIM_NUM; i++) {
            printf("(%lf, %lf) ", query.intval[i].low, query.intval[i].high);
        }
        printf("\n");
        usleep(time_gap * 1000);
    }

    fclose(fp);
    close(socketfd);

    return TRUE;
}


int query(const char *entry_ip, char *file_name, int time_gap) {
    char file_path[MAX_FILE_NAME];
    snprintf(file_path, MAX_FILE_NAME, "%s/%s", DATA_DIR, file_name);

    if(strcmp(file_name, "range_query") == 0){
        printf("begin range query.\n");
        send_query(entry_ip, file_path, time_gap);
        printf("finish range query.\n");
    } else if(strcmp(file_name, "nn_query") == 0){
        printf("begin nn query.\n");
        send_query(entry_ip, file_path, time_gap);
        printf("finish nn query.\n");
    } else {
        printf("query file not found.\n");
        return FALSE;
    }

    return TRUE;
}

/*int main(int argc, char **argv){
    if (argc < 4) {
        printf("usage: %s leader_ip query_file time_gap\n", argv[0]);
        exit(1);
    }
    range_query(argv[1], argv[2], atoi(argv[3]));
    return 0;
}*/


