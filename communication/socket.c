/*
 * socket.c
 *
 *  Created on: Sep 12, 2013
 *      Author: meibenjin
 */

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<time.h>
#include<unistd.h>

#include"socket.h"
#include"logs/log.h"

__asm__(".symver memcpy,memcpy@GLIBC_2.2.5");

// initial a socket address
static void init_socket_addr(struct sockaddr_in *socket_addr, int port);

void init_socket_addr(struct sockaddr_in *sock_addr, int port) {
	bzero(sock_addr, sizeof(struct sockaddr_in));
	sock_addr->sin_family = AF_INET;
	sock_addr->sin_addr.s_addr = htons(INADDR_ANY);
	sock_addr->sin_port = htons(port);
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

int new_client_socket(const char *ip, int port) {
	struct sockaddr_in client_addr;

	//initial client address
	init_socket_addr(&client_addr, port);
	if (FALSE == set_server_ip(&client_addr, ip)) {
		return FALSE;
	}

	int client_socket = socket(AF_INET, SOCK_STREAM, 0);
	//int client_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (client_socket < 0) {
		fprintf(stderr, "%s: socket()\n", strerror(errno));
		return FALSE;
	}

    int snd_buf = 32 * 1024;
    setsockopt(client_socket, SOL_SOCKET, SO_SNDBUF, (const char*)&snd_buf, sizeof(int));

	if (connect(client_socket, (struct sockaddr *) &client_addr,
			sizeof(struct sockaddr)) < 0) {
		fprintf(stderr, "%d, %s: connect()\n", errno, strerror(errno));
		return FALSE;
	}
	return client_socket;
}

int new_server_socket(int port) {
	struct sockaddr_in server_addr;

	//initial server address
	init_socket_addr(&server_addr, port);

	//create server socket. type: TCP stream protocol
	int server_socket = socket(AF_INET, SOCK_STREAM, 0);
	//int server_socket = socket(AF_INET, SOCK_DGRAM, 0);
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

    // set tcp protocol recv buffer
    int rcv_buf = 32 * 1024;
    setsockopt(server_socket, SOL_SOCKET, SO_RCVBUF, (const char*)&rcv_buf, sizeof(int));

	if (listen(server_socket, LISTEN_QUEUE_LENGTH) < 0) {
		fprintf(stderr, "%s: listen()\n", strerror(errno));
		return FALSE;
	}

	return server_socket;
}

int set_nonblocking(int socketfd) {
    int opts;
    opts = fcntl(socketfd, F_GETFL);
    if(opts < 0) {
        fprintf(stderr, "fcntl(socket, GETFL)\n");
        return FALSE;
    }

    opts = opts | O_NONBLOCK;
    if(fcntl(socketfd, F_SETFL, opts) < 0) {
        fprintf(stderr, "fcntl(socket, SETFL, opts)\n");
        return FALSE;
    }
    return TRUE;
}

int set_blocking(int socketfd) {
    int opts;
    opts = fcntl(socketfd, F_GETFL);
    if(opts < 0) {
        fprintf(stderr, "fcntl(socket, GETFL)\n");
        return FALSE;
    }

    opts = opts & ~O_NONBLOCK;
    if(fcntl(socketfd, F_SETFL, opts) < 0) {
        fprintf(stderr, "fcntl(socket, SETFL, opts)\n");
        return FALSE;
    }
    return TRUE;
}

int accept_connection(int socketfd) {
	struct sockaddr_in client_addr;
	socklen_t length = sizeof(client_addr);
	int conn_socket;

	// accept a socket connection from client
	conn_socket = accept(socketfd, (struct sockaddr*) &client_addr, &length);
	if (conn_socket < 0) {
		//fprintf(stderr, "%s: accept()\n", strerror(errno));
		return FALSE;
	}

	return conn_socket;
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

int send_data(int socketfd, void *data, size_t len) {
    size_t nsend = 0;
    int isend = -1;
    while (nsend < len){
        isend = send(socketfd, data + nsend, len - nsend, 0);
        if(isend < 0){
            if(errno != EINTR && errno != EAGAIN) {
                nsend = 0;
                break;
            }
            continue;
        }else if(isend == 0) {
            nsend = 0;
            break;
        }else {
          nsend += isend;
        } 
    }
    return nsend;
}

int recv_data(int socketfd, void *data, size_t len) {
    size_t nrecv = 0;
    int irecv = -1;
    while (nrecv < len){
        irecv = recv(socketfd, data + nrecv, len - nrecv, 0);
        if(irecv < 0){
            if(errno != EINTR && errno != EAGAIN) {
                nrecv = 0;
                //write_log(ERROR_LOG, "recv_safe: network error.\n");
                break;
            }
            continue;
        }else if(irecv == 0) {
            nrecv = 0;
            break;
        }else {
          nrecv += irecv;
        } 
    }
    return nrecv;
}




