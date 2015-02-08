/*
 * connection_mgr.h
 *
 *  Created on: Jan 16, 2015
 *      Author: meibenjin
 */

#ifndef CONNECTION_MGR_H_
#define CONNECTION_MGR_H_

#include"utils.h"
#include"connection.h"

#pragma pack(push)
#pragma pack(4)

// define coming connections' I/O event handler pointer
typedef int (*event_func)(connection_t);

typedef struct connection_mgr{
    // connections arrary
    struct connection_st conn_table[CONN_MAXFD];
    event_func read_event_func;
    event_func write_event_func;
    
    // running flag;
    int running;
}connection_mgr, *connection_mgr_t;

#pragma pack(pop)

//TODO this function should put into another module
/* register an socket into worker epoll with events type being set
 * input: 
 *      index: the index of worker_epfd
 *      socketfd: socket fd that will be registered into worker epoll
 *      events: the event type of socket fd 
 * */
int register_to_worker_epoll(int index, int sockfd, uint32_t event_types);

// create a new connection manager instance
connection_mgr* new_connection_mgr();

/* initialize connection manager
 * set connections I/O event handlers as well
 * 
 * */
void init_connection_mgr(connection_mgr_t conn_mgr, \
        event_func rfunc, event_func wfunc);

/* start several threads to handle 
 * different type of coming connections
 * includes manaual task/compute task/fast task
 * manaual task: tasks with a little data to transform, will be processed fast as well
 * compute task: taskss with more data compare with manual tasks, and cost more process time
 * fast task: only used for heartbeating currently;
 * 
 * */
void run_connection_mgr(connection_mgr_t conn_mgr);

void stop_connection_mgr(connection_mgr_t conn_mgr);

//////////////////////////////////////////////////////////////////////


// put here temporarily

// implement connection manager's read event function 
int handle_read_event(connection_t conn);

// implement connection manager's write event function 
int handle_write_event(connection_t conn);

int async_send_data(connection_t conn, const char *data, uint32_t data_len);

#endif
