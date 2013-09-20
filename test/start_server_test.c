/*
 * socket_test.c
 *
 *  Created on: Sep 12, 2013
 *      Author: meibenjin
 */

#include<stdio.h>
#include"../socket/socket_server.h"
#include"../torus_node.h"

int main(int argc, char **argv)
{
	int server_socket;

    init_torus_node(local_torus_node);

    server_socket = start_server_socket();
	return 0;
}
