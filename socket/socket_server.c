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

int init_server_addr(struct sockaddr_in *server_addr) {
	bzero(server_addr, sizeof(server_addr));
	server_addr->sin_family = AF_INET;
	server_addr->sin_addr.s_addr = htons(INADDR_ANY);
	server_addr->sin_port = htons(LISTEN_PORT);

	return SUCESS;
}

int new_server_socket() {
	struct sockaddr_in server_addr;

	//init server address
	init_server_addr(&server_addr);

	//create server socket. type: TCP stream protocol
	int server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket < 0) {
		fprintf(stderr, "%s: socket()\n", strerror(errno));
		return FAILED;
	}

	//bind server_addr with server_socket descriptor
	int ret = bind(server_socket, (struct sockaddr*) &server_addr,
			sizeof(struct sockaddr));

	if (ret < 0) {
		fprintf(stderr, "%s: bind()\n", strerror(errno));
		return FAILED;
	}

	return server_socket;
}

int accept_connection(int socketfd) {
	struct sockaddr_in client_addr;
	socklen_t length = sizeof(client_addr);
	int conn_socket;

	// accept a socket connection from client
	conn_socket = accept(socketfd, (struct sockaddr*) &client_addr, &length);

	if (conn_socket < 0) {
		fprintf(stderr, "%s: accept()\n", strerror(errno));
		return FAILED;
	}

	printf("accept a connection:%s\n", inet_ntoa(client_addr.sin_addr));

	return conn_socket;
}

int process_request(int socketfd) {
<<<<<<< HEAD
    //char buffer[SOCKET_BUF_SIZE];
    //bzero(buffer, sizeof(SOCKET_BUF_SIZE));
    
    message msg;
    memset(&msg, 0, sizeof(message));
    
    ssize_t recv_len = -1;
    recv_len = recv(socketfd, (void *)&msg, sizeof(message), 0);
    
    if (recv_len < 0) {
        fprintf(stderr, "%s: recv()\n", strerror(errno));
        return FAILED;
    }
    
    if (recv_len > 0) {
        print_message(msg);
    }
    
    return SUCESS;
=======
	//char buffer[SOCKET_BUF_SIZE];
	//bzero(buffer, sizeof(SOCKET_BUF_SIZE));
	
	message msg;
	memset(&msg, 0, sizeof(message));

	ssize_t recv_len = -1;

	recv_len = recv(socketfd, (void *)&msg, sizeof(message), 0);

	if (recv_len < 0) {
		fprintf(stderr, "%s: recv()\n", strerror(errno));
		return FAILED;
	}

	if (recv_len > 0) {
		print_message(msg);
	}
	return SUCESS;
>>>>>>> 49b7b798d18f398bdf7a859fba523fff9f46c850
}

int start_server_socket(int server_socket) {
	// listen connections from client
	if (listen(server_socket, LISTEN_QUEUE_LENGTH) < 0) {
		fprintf(stderr, "%s: bind()\n", strerror(errno));
		return FAILED;
	}

	printf("start server!\n");

	int is_run = 1;
	while (is_run) {
		int conn_socket;
		conn_socket = accept_connection(server_socket);
		if (conn_socket == FAILED) {
			// TODO: handle accept connection failed
			continue;
		}

		process_request(conn_socket);
	}
	return SUCESS;
}

void print_message(message msg) {
	printf("op:%d\n", msg.OP);
	printf("src:%s\n", msg.src);
	printf("dst:%s\n", msg.dst);
}

