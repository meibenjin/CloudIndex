/*
 * message_handler.c
 *
 *  Created on: Jan 21, 2015
 *      Author: meibenjin
 */

#include<stdlib.h>
#include<assert.h>

#include"logs/log.h"
#include"message_handler.h"

message_mgr_t the_msg_mgr;

message_mgr* new_message_mgr() {
	message_mgr* new_msg_mgr;
	new_msg_mgr = (message_mgr *) malloc(sizeof(struct message_mgr));
	if (new_msg_mgr == NULL) {
		write_log(ERROR_LOG,
				"new_message_mgr: malloc for message manager failed.\n");
		return NULL;
	}
	new_msg_mgr->msg_num = 0;
	return new_msg_mgr;
}

int register_message_handler1(OP op, int (*handle_func1)(connection_t, message)) {
    assert(the_msg_mgr != NULL);

    int i = the_msg_mgr->msg_num;
    if(i == MAX_MSG_NUM) {
        write_log(ERROR_LOG, "register_message_handler1: message num exceed %d.\n", MAX_MSG_NUM);
        return FALSE;
    }

    message_handler *msg_h = &the_msg_mgr->handler[i];
    msg_h->op = op;
	msg_h->process1 = handle_func1;

	the_msg_mgr->msg_num++;
    return TRUE;
}

int register_message_handler2(OP op, int (*handle_func2)(message)) {
    assert(the_msg_mgr != NULL);

    int i = the_msg_mgr->msg_num;
    if(i == MAX_MSG_NUM) {
        write_log(ERROR_LOG, "register_message_handler2: message num exceed %d.\n", MAX_MSG_NUM);
        return FALSE;
    }

    message_handler *msg_h = &the_msg_mgr->handler[i];
    msg_h->op = op;
	msg_h->process2 = handle_func2;

	the_msg_mgr->msg_num++;
    return TRUE;
}

message_handler* find_message_handler(OP op) {
    assert(the_msg_mgr != NULL);

    int i, num = the_msg_mgr->msg_num;
    message_handler *msg_h = NULL;
    for(i = 0; i < num; i++) {
        msg_h = &the_msg_mgr->handler[i];
        if(msg_h->op ==  op) {
            return msg_h; 
        }
    }
	return NULL;
}

int remove_message_handler(OP op){
    // TODO implement
    return TRUE;
}


