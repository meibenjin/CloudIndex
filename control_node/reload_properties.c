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
#include "communication/socket.h"
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
        size_t data_len = sizeof(struct global_properties_struct);
        msg.msg_size = calc_msg_header_size() + data_len;
        fill_message(msg.msg_size, RELOAD_PROPERTIES, local_ip, dst_ip, "", (char *)&props, data_len, &msg);
        send_data(socketfd, (void *) &msg, msg.msg_size);
        printf("send reload properties message to torus node %s.\n", dst_ip);
        close(socketfd);
    }
    return TRUE;
}

int main(int argc, char const* argv[]) {
    if (argc < 2) {
        printf("usage: %s cluster_id \n", argv[0]);
        exit(1);
    }

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

