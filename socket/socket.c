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

#include"socket.h"
#include"logs/log.h"

__asm__(".symver memcpy,memcpy@GLIBC_2.2.5");

void init_socket_addr(struct sockaddr_in *sock_addr) {
	bzero(sock_addr, sizeof(struct sockaddr_in));
	sock_addr->sin_family = AF_INET;
	sock_addr->sin_addr.s_addr = htons(INADDR_ANY);
	sock_addr->sin_port = htons(LISTEN_PORT);
}

int set_server_ip(struct sockaddr_in *sock_addr, const char *ip) {
	if (ip == NULL) {
		printf("set_server_ip: ip is null pointer.\n");
		return FALSE;
	}
	if (inet_aton(ip, &sock_addr->sin_addr) == 0) {
		printf("%s: resolve destination ip failed.\n", ip);
		return FALSE;
	}
	return TRUE;
}

int new_client_socket(const char *ip) {
	struct sockaddr_in client_addr;

	//initial client address
	init_socket_addr(&client_addr);
	if (FALSE == set_server_ip(&client_addr, ip)) {
		return FALSE;
	}

	int client_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (client_socket < 0) {
		fprintf(stderr, "%s: socket()\n", strerror(errno));
		return FALSE;
	}

	if (connect(client_socket, (struct sockaddr *) &client_addr,
			sizeof(struct sockaddr)) < 0) {
		printf("%s: connect failed.\n", ip);
		return FALSE;
	}
	return client_socket;
}

int new_server_socket() {
	struct sockaddr_in server_addr;

	//initial server address
	init_socket_addr(&server_addr);

	//create server socket. type: TCP stream protocol
	int server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket < 0) {
		fprintf(stderr, "%s: socket()\n", strerror(errno));
		return FALSE;
	}

	//bind server_addr with server_socket descriptor
	int ret = bind(server_socket, (struct sockaddr*) &server_addr,
			sizeof(struct sockaddr));

	if (ret < 0) {
		fprintf(stderr, "%s: bind()\n", strerror(errno));
		return FALSE;
	}

	if (listen(server_socket, LISTEN_QUEUE_LENGTH) < 0) {
		fprintf(stderr, "%s: listen()\n", strerror(errno));
		return FALSE;
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
		return FALSE;
	}

	//printf("accept a connection:%s\n", inet_ntoa(client_addr.sin_addr));

	return conn_socket;
}

int send_reply(int socketfd, struct reply_message reply_msg) {
	if (SOCKET_ERROR
			== send(socketfd, (void *) &reply_msg, sizeof(struct reply_message),
					0)) {
		// TODO do something if send failed
		printf("send_reply: send reply failed.\n");
		return FALSE;
	}
	return TRUE;
}

int receive_reply(int socketfd, struct reply_message *reply_msg) {
	if (reply_msg == NULL) {
		printf("receive_reply: reply_msg is null pointer.\n");
		return FALSE;
	}

	ssize_t recv_len = -1;
	recv_len = recv(socketfd, reply_msg, sizeof(struct reply_message), 0);
	if (recv_len <= 0) {
		fprintf(stderr, "%s: recv()\n", strerror(errno));
		return FALSE;
	}

	return TRUE;
}

void fill_message(OP op, const char *src_ip, const char *dst_ip, const char *stamp, const char *data, message *msg){
	msg->op = (OP)op;
	strncpy(msg->src_ip, src_ip, IP_ADDR_LENGTH);
	strncpy(msg->dst_ip, dst_ip, IP_ADDR_LENGTH);
	memcpy(msg->stamp, stamp, STAMP_SIZE);
	memcpy(msg->data, data, DATA_SIZE);
}

int forward_message(struct message msg, int need_reply) {
	int socketfd;
	socketfd = new_client_socket(msg.dst_ip);
	if (FALSE == socketfd) {
		return FALSE;
	}

	int ret = FALSE;

	if (TRUE == send_message(socketfd, msg)) {
		printf("\tforward message: %s -> %s\n", msg.src_ip, msg.dst_ip);

        #ifdef WRITE_LOG 
            //write log
            char buf[1024];
            memset(buf, 0, 1024);
            sprintf(buf, "\tforward message: %s -> %s\n", msg.src_ip, msg.dst_ip);
            write_log(TORUS_NODE_LOG, buf);
        #endif

		if (need_reply) {
			struct reply_message reply_msg;
			if (TRUE == receive_reply(socketfd, &reply_msg)) {
				if (SUCCESS == reply_msg.reply_code) {
					ret = TRUE;
				} else {
					ret = FALSE;
				}
			} else {
				ret = FALSE;
			}
		}
	}
	close(socketfd);

	return ret;
}

int send_message(int socketfd, struct message msg) {
	if (SOCKET_ERROR
			== send(socketfd, (void *) &msg, sizeof(struct message), 0)) {
		// TODO do something if send failed
		printf("send_message: %s send message failed.\n", msg.src_ip);
		return FALSE;
	}
	return TRUE;
}

int receive_message(int socketfd, struct message *msg) {
	if (msg == NULL) {
		printf("receive_message: msg is null pointer.\n");
		return FALSE;
	}

	ssize_t recv_len = -1;
	recv_len = recv(socketfd, (void *) msg, sizeof(struct message), 0);
	if (recv_len <= 0) {
		fprintf(stderr, "%s: recv()\n", strerror(errno));
		return FALSE;
	}

	return TRUE;
}

// TODO get local ip without ifr_name
int get_local_ip(char *ip) {
	if (ip == NULL) {
		printf("get_local_ip: ip is null pointer.\n");
		return FALSE;
	}

	int socketfd;
	struct ifreq ifr;
	socketfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (socketfd < 0) {
		fprintf(stderr, "%s: socket()\n", strerror(errno));
		return FALSE;
	}

	int ret = FALSE;
	strncpy(ifr.ifr_name, "eth0", IFNAMSIZ);
	if (ioctl(socketfd, SIOCGIFADDR, &ifr) >= 0) {
		strncpy(ip,
				inet_ntoa(((struct sockaddr_in*) &(ifr.ifr_addr))->sin_addr),
				IP_ADDR_LENGTH);
		ret = TRUE;
	} else {
		printf("get_local_ip: get local ip failed.\n");
		ret = FALSE;
	}
	close(socketfd);
	return ret;
}

void print_message(struct message msg) {
	printf("op:%d\n", msg.op);
	printf("src:%s\n", msg.src_ip);
	printf("dst:%s\n", msg.dst_ip);
}
