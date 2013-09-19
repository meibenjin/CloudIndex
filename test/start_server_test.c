/*
 * socket_test.c
 *
 *  Created on: Sep 12, 2013
 *      Author: meibenjin
 */

#include<stdio.h>
#include"../socket/socket_server.h"

int main(int argc, char **argv)
{
	int server_socket;
    server_socket = start_server_socket();
	return 0;
}
