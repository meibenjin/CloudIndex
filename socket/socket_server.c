/*
 * socket_server.c
 *
 *  Created on: Sep 12, 2013
 *      Author: meibenjin
 */

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>

#include"socket_server.h"

int init_server_addr(struct sockaddr_in *server_addr)
{
	bzero(server_addr, sizeof(server_addr));
	server_addr->sin_family = AF_INET;
	server_addr->sin_addr.s_addr = htons(INADDR_ANY);
	server_addr->sin_port = htons(LISTEN_PORT);

	return TRUE;
}

int new_server_socket()
{
	struct sockaddr_in server_addr;

	//init server address
	init_server_addr(&server_addr);

	//create server socket. type: TCP stream protocol
	int server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket < 0)
    {
		fprintf(stderr, "%s: socket()\n", strerror(errno));
		return FALSE;
	}

	//bind server_addr with server_socket descriptor
	int ret = bind(server_socket, (struct sockaddr*) &server_addr,
			sizeof(struct sockaddr));

	if (ret < 0)
    {
		fprintf(stderr, "%s: bind()\n", strerror(errno));
		return FALSE;
	}

	return server_socket;
}

int accept_connection(int socketfd)
{
	struct sockaddr_in client_addr;
	socklen_t length = sizeof(client_addr);
	int conn_socket;

	// accept a socket connection from client
	conn_socket = accept(socketfd, (struct sockaddr*) &client_addr, &length);

	if (conn_socket < 0)
    {
		fprintf(stderr, "%s: accept()\n", strerror(errno));
		return FALSE;
	}

	printf("accept a connection:%s\n", inet_ntoa(client_addr.sin_addr));

	return conn_socket;
}

int update_torus(struct message msg)
{
    set_node_ip(&local_torus_node, msg.dst_ip);
    // TODO create torus (need controll send node info)
    //memcpy(&neighbors_num, (void*)&msg.data, sizeof(int));
}

int process_message(struct message msg)
{
    int ret;
    switch(msg.op)
    {
        case UPDATE_TORUS:
            update_torus(msg);
            ret = SUCCESS;
        case READ:
            ret = SUCCESS;
        case WRITE:
            ret = SUCCESS;
        default:
            ret = WRONG_OP;
    }
    return ret;
}

int process_request(int socketfd)
{
	//char buffer[SOCKET_BUF_SIZE];
	//bzero(buffer, sizeof(SOCKET_BUF_SIZE));

	struct message msg;
	memset(&msg, 0, sizeof(struct message));

	ssize_t recv_len = -1;

	recv_len = recv(socketfd, (void *)&msg, sizeof(struct message), 0);

	if (recv_len <= 0)
    {
		fprintf(stderr, "%s: recv()\n", strerror(errno));
		return FALSE;
	}
    else
    {
        process_message(msg);
    }

	return TRUE;
}

int start_server_socket()
{

	int server_socket;
	server_socket = new_server_socket();
	if (server_socket == FALSE)
    {
		return FALSE;
	}

	// listen connections from client
	if (listen(server_socket, LISTEN_QUEUE_LENGTH) < 0)
    {
		fprintf(stderr, "%s: bind()\n", strerror(errno));
		return FALSE;
	}

	printf("start server!\n");

	int is_run = 1;
	while (is_run)
    {
		int conn_socket;
		conn_socket = accept_connection(server_socket);
		if (conn_socket == FALSE)
        {
			// TODO: handle accept connection failed
			continue;
		}
		process_request(conn_socket);

		close(conn_socket);
	}
}

void print_message(message msg)
{
	printf("op:%d\n", msg.op);
	printf("src:%s\n", msg.src_ip);
	printf("dst:%s\n", msg.dst_ip);
}

