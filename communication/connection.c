/*
 * connection.c
 *
 *  Created on: Jan 22, 2015
 *      Author: meibenjin
 */

#include<string.h>
#include<unistd.h>

#include "connection.h"

void init_connection(connection_t conn) {
    conn->socketfd = 0;
    conn->index = 0;
    conn->used = 0;
    conn->roff = 0;
    memset(conn->rbuf, 0, CONN_BUF_SIZE);
    conn->woff = 0;
    memset(conn->wbuf, 0, CONN_BUF_SIZE);
}

void set_connection_info(connection_t conn, int conn_socket, int epoll_idx, int in_use) {
    conn->socketfd = conn_socket; 
    conn->index = epoll_idx; 
    conn->used = in_use;
}

void close_connection(connection_t conn) {
    conn->used = 0;
    conn->roff = 0;
    //epoll_ctl(worker_epfd[conn->index], EPOLL_CTL_DEL, conn->socketfd, &ev);
    close(conn->socketfd);
}

