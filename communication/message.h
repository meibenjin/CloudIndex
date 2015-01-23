/*
 * message.h
 *
 *  Created on: Jan 14, 2015
 *      Author: meibenjin
 */

#ifndef MESSAGE_H_
#define MESSAGE_H_

#include"utils.h"

typedef struct message {
    size_t msg_size; 
	OP op;
	char src_ip[IP_ADDR_LENGTH];
	char dst_ip[IP_ADDR_LENGTH];
	char stamp[STAMP_SIZE];
    char data[DATA_SIZE];
}message, *message_p;

/* reply message for transport among torus nodes
 * op: operation code
 * stamp unique request stamp for global system
 * reply_code: message process result
 */
typedef struct reply_message{
	OP op;
	char stamp[STAMP_SIZE];
	REPLY_CODE reply_code;
}reply_message;


void fill_message(size_t msg_size,  OP op, const char *src_ip, \
                  const char *dst_ip, const char *stamp, const char *data, \
                  size_t data_len, message *msg);

size_t calc_msg_header_size();

void print_message(struct message msg);

// send data, without socket fd
int send_message(struct message msg);

int receive_message(int socketfd, struct message *msg);

int send_reply(int socketfd, struct reply_message reply_msg);

int receive_reply(int socketfd, struct reply_message *reply_msg);

#endif


