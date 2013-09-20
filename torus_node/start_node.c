/*
 * torus_node.c
 *
 *  Created on: Sep 16, 2013
 *      Author: meibenjin
 */

#include<stdio.h>
#include<stdlib.h>

#include"torus_node.h"
#include"../socket/socket_server.h"

int main(int argc, char **argv)
{
	int server_socket;

    init_torus_node(&local_torus_node);

    server_socket = start_server_socket();
	return 0;
}

