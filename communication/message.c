/*
 * message.c
 *
 *  Created on: Jan 14, 2015
 *      Author: meibenjin
 */

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<time.h>

#include"socket.h"
#include"message.h"
#include"logs/log.h"

__asm__(".symver memcpy,memcpy@GLIBC_2.2.5");

void fill_message(size_t msg_size, OP op, const char *src_ip, \
                  const char *dst_ip, const char *stamp, const char *data, \
                  size_t data_len, struct message *msg)
{
	msg->op = op;
	strncpy(msg->src_ip, src_ip, IP_ADDR_LENGTH);
	strncpy(msg->dst_ip, dst_ip, IP_ADDR_LENGTH);
	strncpy(msg->stamp, stamp, STAMP_SIZE);

    msg->msg_size = msg_size + 1;
	memcpy(msg->data, data, data_len);
    msg->data[data_len] = '\0';
}

inline size_t calc_msg_header_size() {
    return sizeof(struct message) - DATA_SIZE;
    //return sizeof(size_t) + sizeof(enum OP) + 2 * IP_ADDR_LENGTH + STAMP_SIZE; 
}

void print_message(struct message msg) {
	printf("op:%d\n", msg.op);
	printf("src:%s\n", msg.src_ip);
	printf("dst:%s\n", msg.dst_ip);
}

int send_message(struct message msg) {
	int socketfd;
	socketfd = new_client_socket(msg.dst_ip, MANUAL_WORKER_PORT);
	if (FALSE == socketfd) {
		printf("send_message: create socket failed.\n");
		return FALSE;
	}

    int ret = send_data(socketfd, (void *) &msg, msg.msg_size);
    if(ret < msg.msg_size) {
		printf("%s socket error: %s.\n", msg.src_ip, strerror(errno));
		return FALSE;
    }
	return TRUE;
}

int receive_message(int socketfd, struct message *msg) {
	if (msg == NULL) {
		printf("receive_message: msg is null pointer.\n");
		return FALSE;
	}

    // get message k
    size_t msg_size;
	ssize_t recv_len = -1;
    recv_len = recv_data(socketfd, (void *) &msg_size, sizeof(msg_size));
    if(recv_len <= 0) {
		return FALSE;
    }
    msg->msg_size = msg_size;

    recv_len = -1;
	recv_len = recv_data(socketfd, ((void *) msg) + sizeof(msg_size), msg_size - sizeof(msg_size));
	if (recv_len <= 0) {
		//fprintf(stderr, "%s: recv()\n", strerror(errno));
		return FALSE;
	}

	return TRUE;
}

int send_reply(int socketfd, struct reply_message reply_msg) {

    int ret = send_data(socketfd, (void *)&reply_msg, sizeof(reply_msg));
    if(ret < sizeof(reply_msg)) {
		printf("send_reply: send reply failed.\n");
		return FALSE;
    }

	/*if (SOCKET_ERROR
			== send(socketfd, (void *)&reply_msg, sizeof(struct reply_message),
					0)) {
		// TODO do something if send failed
		printf("send_reply: send reply failed.\n");
		return FALSE;
	}*/
	return TRUE;
}

int receive_reply(int socketfd, struct reply_message *reply_msg) {
	if (reply_msg == NULL) {
		printf("receive_reply: reply_msg is null pointer.\n");
		return FALSE;
	}

    int ret = recv_data(socketfd, (void *)reply_msg, sizeof(struct reply_message));
    if(ret < sizeof(struct reply_message)) {
		printf("receive_reply: send reply failed.\n");
		return FALSE;
    }

	/*ssize_t recv_len = -1;
	recv_len = recv(socketfd, reply_msg, sizeof(struct reply_message), 0);
	if (recv_len <= 0) {
		fprintf(stderr, "%s: recv()\n", strerror(errno));
		return FALSE;
	}*/

	return TRUE;
}

