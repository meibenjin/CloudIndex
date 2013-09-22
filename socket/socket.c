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

int do_update_torus(struct message msg)
{
    int i;
    int nodes_num = 0;

    memcpy(&nodes_num, msg.data, sizeof(int));
    if( nodes_num <= 0)
    {
        printf("do_update_torus: torus nodes num is wrong\n");
        return FALSE; 
    }

    struct node_info nodes[nodes_num];

    for(i = 0; i < nodes_num; ++i)
    {
        memcpy(&nodes[i], (void*)(msg.data + sizeof(int) + sizeof(struct node_info) * i), sizeof(struct node_info));
    }

    // create torus node from serval nodes
    if(FALSE == create_torus_node(&local_torus_node, nodes))
    {
        return FALSE;
    }
    return TRUE;
}

int process_message(struct message msg)
{
    int reply_code;
    switch(msg.op)
    {
        case UPDATE_TORUS:
            if (TRUE == do_update_torus(msg))
                reply_code= SUCCESS;
            else
                reply_code= FAILED;
        default:
            reply_code= WRONG_OP;
    }
    return reply_code;
}

int process_request(int socketfd)
{

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
        int reply_code = process_message(msg);
        // TODO reply the client
        char send_buf[MAX_REPLY_SIZE];
        memcpy(send_buf, (void *)&reply_code, MAX_REPLY_SIZE);
        if(SOCKET_ERROR == send(socketfd, (void *)send_buf, sizeof(MAX_REPLY_SIZE), 0))
        {
            // TODO do something if send failed
            printf("send reply failed\n");
            return FALSE;
        }
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

