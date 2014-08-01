/*
 * reload_properties.c
 *
 *  Created on: Aug 1, 2014
 *      Author: meibenjin
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "socket/socket.h"
#include "config/config.h"
#include "torus_node/torus_node.h"

int send_properties(struct global_properties_struct props, int cluster_id) {
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
        msg.op = RELOAD_PROPERTIES;
        strncpy(msg.src_ip, local_ip, IP_ADDR_LENGTH);
        strncpy(msg.dst_ip, dst_ip, IP_ADDR_LENGTH);
        strncpy(msg.stamp, "", STAMP_SIZE);
        memcpy(msg.data, &props, sizeof(struct global_properties_struct));
        send_safe(socketfd, (void *) &msg, sizeof(struct message), 0);
        printf("send reload properties message to torus node %s.\n", dst_ip);
        close(socketfd);
    }
    return TRUE;
}

int main(int argc, char const* argv[]) {
    // read ip pool from file 
	if (FALSE == read_torus_ip_list()) {
		exit(1);
	}

    struct global_properties_struct props;
    // read properties file 
	if (FALSE == read_properties(&props)) {
        printf("read properties file failed.\n");
        return 0;
	}

    // read cluster partitions from file 
    if(FALSE == read_cluster_partitions()) {
        exit(1);
    }

    int cluster_id = atoi(argv[1]);
    send_properties(props, cluster_id);
    return 0;
}

