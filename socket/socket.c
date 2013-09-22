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

void init_socket_addr(struct sockaddr_in *sock_addr) {
	bzero(sock_addr, sizeof(struct sockaddr_in));
	sock_addr->sin_family = AF_INET;
	sock_addr->sin_addr.s_addr = htons(INADDR_ANY);
	sock_addr->sin_port = htons(LISTEN_PORT);

}

int set_server_ip(struct sockaddr_in *sock_addr, char *ip) {
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

int new_client_socket(char *ip) {
	struct sockaddr_in client_addr;

	//initial client address
	init_socket_addr(&client_addr);
	if(FALSE == set_server_ip(&client_addr, ip)) {
        return FALSE;
    }


	int client_socket = socket(AF_INET, SOCK_STREAM, 0);

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

	printf("accept a connection:%s\n", inet_ntoa(client_addr.sin_addr));

	return conn_socket;
}

int send_reply(int socketfd, int reply_code) {
	char reply_buf[REPLY_SIZE];
	memcpy(reply_buf, (void *) &reply_code, REPLY_SIZE);
	if (SOCKET_ERROR == send(socketfd, (void *) reply_buf, REPLY_SIZE, 0)) {
		// TODO do something if send failed
		printf("reply: send reply failed.\n");
		return FALSE;
	}
	return TRUE;
}

int receive_reply(int socketfd) {
	char reply_buf[REPLY_SIZE];
	memset(reply_buf, 0, REPLY_SIZE);

	// receive reply after send torus nodes
	ssize_t recv_len = -1;

	recv_len = recv(socketfd, reply_buf, REPLY_SIZE, 0);
	if (recv_len <= 0) {
		fprintf(stderr, "%s: recv()\n", strerror(errno));
		return FALSE;
	}

	int reply = FAILED;
	memcpy((void *) &reply, reply_buf, REPLY_SIZE);
	if (SUCCESS == reply) {
		return TRUE;
	} else {
		return FALSE;
	}
	return TRUE;
}

int send_message(int socketfd, const struct message msg) {
	if (SOCKET_ERROR
			== send(socketfd, (void *) &msg, sizeof(struct message), 0)) {
		// TODO do something if send failed
		printf("%s: send torus nodes failed.\n", msg.dst_ip);
		close(socketfd);
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

int get_local_ip(char *ip) {
	if (ip == NULL) {
		printf("get_local_ip: ip is null pointer.\n");
		return FALSE;
	}

	int socketfd;
	struct ifconf conf;
	struct ifreq *ifr;
	char buff[BUFSIZ];
	int num = 0;

	socketfd = socket(PF_INET, SOCK_DGRAM, 0);
	conf.ifc_len = BUFSIZ;
	conf.ifc_buf = buff;

	ioctl(socketfd, SIOCGIFCONF, &conf);
	num = conf.ifc_len / sizeof(struct ifreq);
	ifr = conf.ifc_req;

	if (num > 0) {
		struct sockaddr_in *sin = (struct sockaddr_in *) (&ifr->ifr_addr);
		ioctl(socketfd, SIOCGIFFLAGS, ifr);
		if (((ifr->ifr_flags & IFF_LOOPBACK) == 0)
				&& (ifr->ifr_flags & IFF_UP)) {
			strncpy(ip, inet_ntoa(sin->sin_addr), IP_ADDR_LENGTH);
		}
	} else {
		printf("get_local_ip: get local ip failed.\n");
		return FALSE;
	}
	return TRUE;
}

void print_message(message msg) {
	printf("op:%d\n", msg.op);
	printf("src:%s\n", msg.src_ip);
	printf("dst:%s\n", msg.dst_ip);
}

