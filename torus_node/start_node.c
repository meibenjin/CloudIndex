/*
 * start_node.c
 *
 *  Created on: Sep 18, 2013
 *      Author: meibenjin
 */

#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include"utils.h"
#include"torus_node.h"
#include"server.h"
#include"skip_list/skip_list.h"
#include"logs/log.h"

//__asm__(".symver memcpy,memcpy@GLIBC_2.2.5");

int main(int argc, char **argv) {

	//write log
	char ip[IP_ADDR_LENGTH];
	get_local_ip(ip);
	sprintf(ip, "%s\n", ip);
	write_log(TORUS_NODE_LOG, ip);

	printf("start torus node.\n");
	write_log(TORUS_NODE_LOG, "start torus node.\n");

	int server_socket;
	should_run = 1;

	// new a torus node
	the_torus_node = *new_torus_node();

	// create a new request list
	req_list = new_request();

	//init_skip_list();

	server_socket = new_server();
	if (server_socket == FALSE) {
		exit(1);
	}
	printf("start server.\n");
	write_log(TORUS_NODE_LOG, "start server.\n");

	while (should_run) {
		int conn_socket;
		conn_socket = accept_connection(server_socket);
		if (conn_socket == FALSE) {
			// TODO: handle accept connection failed
			continue;
		}

		struct message msg;
		memset(&msg, 0, sizeof(struct message));

		// receive message through the conn_socket
		if (TRUE == receive_message(conn_socket, &msg)) {
			process_message(conn_socket, msg);
		} else {
			//  TODO: handle receive message failed
			printf("receive message failed.\n");
		}
		close(conn_socket);
	}

	return 0;
}

