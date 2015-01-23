/*
 * message_handler.h
 *
 *  Created on: Jan 21, 2015
 *      Author: meibenjin
 */

#ifndef MESSAGE_HANDLER_H_
#define MESSAGE_HANDLER_H_


#include"utils.h"
#include"connection.h"
#include"message.h"

// define coming connections' I/O event handler pointer
//typedef int (*handle_func1)(connection_t, message msg);
//typedef int (*handle_func2)(message msg);

typedef struct message_handler {
    OP op;
    union {
        int (*process1)(connection_t, message) ;
        int (*process2)(message);
    };
}message_handler;

typedef struct message_mgr {
	message_handler handler[MAX_MSG_NUM];
	size_t msg_num;
}message_mgr, *message_mgr_t;

// create a new message collection manager instance
message_mgr* new_message_mgr();

//TODO this two function will be combine together
int register_message_handler1(OP op, int (*process1)(connection_t, message));
int register_message_handler2(OP op, int (*processs2)(message));

message_handler* find_message_handler(OP op);

int remove_message_handler(OP op);


// the message collection manager
extern message_mgr_t the_msg_mgr;

#endif
