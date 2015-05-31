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

#pragma pack(push)
#pragma pack(4)

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

#pragma pack(pop)

// create a new message collection manager instance
message_mgr* new_message_mgr();

// TODO this two function will be combine together in the future 
// handle_func1 and handle_func2 can use two parameters  
int register_message_handler1(OP op, int (*handle_func1)(connection_t, message));
int register_message_handler2(OP op, int (*handle_func2)(message));

message_handler* find_message_handler(OP op);

int remove_message_handler(OP op);


// the message collection manager
extern message_mgr_t the_msg_mgr;

#endif
