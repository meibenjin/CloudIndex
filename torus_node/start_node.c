/*
 * start_node.c
 *
 *  Created on: Sep 18, 2013
 *      Author: meibenjin
 */

#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include"torus_node.h"

int main(int argc, char **argv) {
	int server_socket;
	should_run = 1;
	init_torus_node(&local_torus_node);

	server_socket = new_server_socket();
	if (server_socket == FALSE) {
		exit(1);
	}
	printf("start server.\n");

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
		if(TRUE == receive_message(conn_socket, &msg)){
			process_message(conn_socket, msg);
		} else{
			//  TODO: handle receive message failed
			printf("receive message failed.\n");
		}
		close(conn_socket);
	}

	return 0;
}

