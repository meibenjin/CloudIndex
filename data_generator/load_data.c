/*
 * load_data.c
 *
 *  Created on: July 30, 2014
 *      Author: meibenjin
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>

#include"socket/socket.h"
#include "config/config.h"
#include "torus_node/torus_node.h"

int notify_load_data(int cluster_id) {
    int i;
    int socketfd;
    //get torus partition
    torus_partitions tp = cluster_partitions[cluster_id];
    int torus_num = tp.p_x * tp.p_y * tp.p_z;

    char dst_ip[IP_ADDR_LENGTH];
    for(i = 0; i < torus_num; i++) {
        strncpy(dst_ip, torus_ip_list[i], IP_ADDR_LENGTH);
        socketfd = new_client_socket(dst_ip, MANUAL_WORKER_PORT);
        if (FALSE == socketfd) {
            printf("create new socket failed!\n");
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
        msg.op = LOAD_DATA;
        strncpy(msg.src_ip, local_ip, IP_ADDR_LENGTH);
        strncpy(msg.dst_ip, dst_ip, IP_ADDR_LENGTH);
        strncpy(msg.stamp, "", STAMP_SIZE);
        strncpy(msg.data, "", DATA_SIZE);
        send_safe(socketfd, (void *) &msg, sizeof(struct message), 0);

        printf("notify torus node %s to load data.\n", dst_ip);

        close(socketfd);
    }
    return TRUE;
}

int main(int argc, char const* argv[]) {
    if (argc < 1) {
        printf("usage: %s cluster_id\n", argv[0]);
        exit(1);
    }

    // read ip pool from file 
	if (FALSE == read_torus_ip_list()) {
		exit(1);
	}

    // read cluster partitions from file 
    if(FALSE == read_cluster_partitions()) {
        exit(1);
    }

    int cluster_id = atoi(argv[1]);
    notify_load_data(cluster_id);
    return 0;
}




