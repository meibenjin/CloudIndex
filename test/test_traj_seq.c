/*
 * test_system_status.c
 *
 *  Created on: July 30, 2014
 *      Author: meibenjin
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>

#include "utils.h"
#include "communication/socket.h"
#include "communication/message.h"
#include "config/config.h"
#include "torus_node/torus_node.h"

int check_traj_seq(int cluster_id) {
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
        msg.msg_size = calc_msg_header_size() + 1;
        fill_message(msg.msg_size, CHECK_TRAJ_SEQ, local_ip, dst_ip, "", "", 1, &msg);
        send_data(socketfd, (void *) &msg, msg.msg_size);

        printf("notify torus node %s to check traj sequence.\n", dst_ip);

        close(socketfd);
    }
    return TRUE;
}

int main(int argc, char const* argv[]) {
    if (argc < 2) {
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
    check_traj_seq(cluster_id);
    return 0;
}

