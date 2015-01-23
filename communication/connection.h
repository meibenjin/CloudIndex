/*
 * connection.h
 *
 *  Created on: Jan 22, 2015
 *      Author: meibenjin
 */

#ifndef CONNECTION_H_
#define CONNECTION_H_

#include "utils.h"

// struct connections
typedef struct connection_st {
    int socketfd;
    // which epoll fd this conn belongs to
    int index; 
    // flag
    int used;
    //read offset
    size_t roff;
    char rbuf[CONN_BUF_SIZE];
    //write offset
    size_t woff;
    char wbuf[CONN_BUF_SIZE];
}*connection_t;

void init_connection(connection_t conn);

void set_connection_info(connection_t conn, int conn_socket, int epoll_idx, int in_use);

void close_connection(connection_t conn);

#endif
