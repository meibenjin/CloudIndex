/*
 * connection_mgr.c
 *
 *  Created on: Jan 16, 2015
 *      Author: meibenjin
 */

#include<stdlib.h>
#include<string.h>
#include<assert.h>
#include<errno.h>
#include<unistd.h>
#include<sys/epoll.h>
#include<pthread.h>


#include "logs/log.h"
#include "socket.h"
#include "connection_mgr.h"
#include "message.h"
#include "message_handler.h"

static int worker_epfd[EPOLL_NUM];

static pthread_t manual_worker_monitor_thread;
static pthread_t fast_worker_monitor_thread;
static pthread_t compute_worker_monitor_thread;

// internal functions 
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

// manual worker monitor thread handler
void *manual_worker_monitor(void *args);

// fast worker monitor thread handler
void *fast_worker_monitor(void *args);

// compute worker monitor thread handler
void *compute_worker_monitor(void *args);

// worker thread threads handler
void *worker(void *args);

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

// functions for each connection
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

static void init_conn_table(connection_mgr_t conn_mgr);

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

static void init_conn_table(connection_mgr_t conn_mgr) {
    int i;
    for(i = 0; i < CONN_MAXFD; i++) {
        init_connection(&conn_mgr->conn_table[i]);
    }
}

int add_to_epoll(int epoll_fd, int sockfd, uint32_t event_types) {
    struct epoll_event ev;
    ev.data.fd = sockfd;
    ev.events = event_types;
    int ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sockfd, &ev);
    if(0 != ret) {
        write_log(ERROR_LOG, "add_to_epoll: add sockfd to epoll failed.\n");
        return FALSE;
    }
    return TRUE;
}

int modify_epoll(int epoll_fd, int sockfd, uint32_t event_types) {
    struct epoll_event ev;
    ev.data.fd = sockfd;
    ev.events = event_types;
    int ret = epoll_ctl(epoll_fd, EPOLL_CTL_MOD, sockfd, &ev);
    if(0 != ret) {
        write_log(ERROR_LOG, "add_to_epoll: add sockfd to epoll failed.\n");
        return FALSE;
    }
    return TRUE;
}

int register_to_worker_epoll(int index, int sockfd, uint32_t event_types) {
    return add_to_epoll(worker_epfd[index], sockfd, event_types);
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

// functions for connection manager
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

void init_connection_mgr(connection_mgr_t conn_mgr, event_func rfunc, event_func wfunc) {
    init_conn_table(conn_mgr);
    conn_mgr->read_event_func = rfunc;
    conn_mgr->write_event_func = wfunc; 
    conn_mgr->running = 0;
}

connection_mgr* new_connection_mgr() {
	connection_mgr *new_conn_mgr;
	new_conn_mgr = (connection_mgr *) malloc(sizeof(struct connection_mgr));
	if (new_conn_mgr == NULL) {
		write_log(ERROR_LOG, "new_connection_mgr: malloc for connection manager failed.\n");
		return NULL;
	}
	return new_conn_mgr;
}

void *manual_worker_monitor(void *args) {
    //get connection manager from args
    connection_mgr_t conn_mgr = (connection_mgr_t)args;

	int manual_worker_socket = new_server_socket(MANUAL_WORKER_PORT);
	if (manual_worker_socket == FALSE) {
        write_log(ERROR_LOG, "start manual-worker server failed.\n");
        exit(1);
	}
	write_log(TORUS_NODE_LOG, "start manual-worker server.\n");

	set_nonblocking(manual_worker_socket);

	// add manual_worker_monitor
	int epfd;
	epfd = epoll_create(MAX_EVENTS);
    add_to_epoll(epfd, manual_worker_socket, EPOLLIN);

    // manual worker index from 0 ~ MANUAL_WORKER - 1
    int index = 0;
	int nfds, conn_socket, i;
    connection_t conn;
	struct epoll_event events[MAX_EVENTS];

	while(conn_mgr->running) {
		nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);
		for(i = 0; i < nfds; i++) {
            while((conn_socket = accept_connection(manual_worker_socket)) > 0) {
                if(conn_socket > CONN_MAXFD) {
                    // TODO handle conn_socket fd overhead CONN_MAXFD
                    write_log(ERROR_LOG, "manual_worker_monitor: conn_socket exceed CONN_MAXFD.\n");
                    close(conn_socket);
                }
                set_nonblocking(conn_socket);
                //ev.data.fd = conn_socket;
                //ev.events = EPOLLIN; //| EPOLLONESHOT;

                /* register conn_socket to worker-pool's 
                 * epoll, not the listen epoll*/
                conn = &(conn_mgr->conn_table[conn_socket]);
                set_connection_info(conn, conn_socket, index, 1);
                add_to_epoll(worker_epfd[index], conn_socket, EPOLLIN | EPOLLONESHOT);

                index = (index + 1) % MANUAL_WORKER;
            }
		} 
	}

    close(manual_worker_socket);
    close(epfd);
    pthread_cancel(pthread_self());
    return NULL;
}

void *fast_worker_monitor(void *args) {
    //get connection manager from args
    connection_mgr_t conn_mgr = (connection_mgr_t)args;
    int fast_worker_socket = new_server_socket(FAST_WORKER_PORT);
	if (fast_worker_socket == FALSE) {
        write_log(TORUS_NODE_LOG, "start fast-worker server failed.\n");
        exit(1);
	}
	write_log(TORUS_NODE_LOG, "start fast-worker server.\n");
	set_nonblocking(fast_worker_socket);

	// add fast_worker_monitor
	int epfd;
	epfd = epoll_create(MAX_EVENTS);
    add_to_epoll(epfd, fast_worker_socket, EPOLLIN);

    // fast worker index from MANUAL_WORKER ~ (MANUAL_WORKER + FAST_WORKER - 1)
    int index = MANUAL_WORKER;
	int nfds, conn_socket, i;
    connection_t conn;
	struct epoll_event events[MAX_EVENTS];

	while(conn_mgr->running) {
		nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);
		for(i = 0; i < nfds; i++) {
            while((conn_socket = accept_connection(fast_worker_socket)) > 0) {
                if(conn_socket > CONN_MAXFD) {
                    // TODO handle conn_socket fd exceed CONN_MAXFD
                    write_log(ERROR_LOG, "faster_worker_monitor: conn_socket exceed CONN_MAXFD.\n");
                    close(conn_socket);
                }
                set_nonblocking(conn_socket);
                //ev.data.fd = conn_socket;
                //ev.events = EPOLLIN; //| EPOLLONESHOT;

                conn = &(conn_mgr->conn_table[conn_socket]);
                set_connection_info(conn, conn_socket, index, 1);
                /* register conn_socket to worker-pool's 
                 * epoll, not the listen epoll*/
                add_to_epoll(worker_epfd[index], conn_socket, EPOLLIN | EPOLLONESHOT);

                index = (index + 1) % (MANUAL_WORKER + FAST_WORKER);
                if(index == 0) {
                    index = MANUAL_WORKER;
                }
            }
		} 
	}

    close(fast_worker_socket);
    close(epfd);
    pthread_cancel(pthread_self());
    return NULL;
}

void *compute_worker_monitor(void *args) {
    //get connection manager from args
    connection_mgr_t conn_mgr = (connection_mgr_t)args;

	int compute_worker_socket = new_server_socket(COMPUTE_WORKER_PORT);
	if (compute_worker_socket == FALSE) {
        write_log(TORUS_NODE_LOG, "start compute-worker server failed.\n");
        exit(1);
	}
	write_log(TORUS_NODE_LOG, "start compute-worker server.\n");
	set_nonblocking(compute_worker_socket);

	int epfd;
	epfd = epoll_create(MAX_EVENTS);
    add_to_epoll(epfd, compute_worker_socket, EPOLLIN | EPOLLET);
	struct epoll_event events[MAX_EVENTS];

    // epoll index from (MANUAL_WORKER + FAST_WORKER) ~ (WORKER_NUM - 1) used for compute worker
    int index = MANUAL_WORKER + FAST_WORKER;
	int nfds, conn_socket, i;
    connection_t conn;

	while(conn_mgr->running) {
		nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);
		for(i = 0; i < nfds; i++) {
            if(events[i].data.fd == compute_worker_socket) {
                while((conn_socket = accept_connection(compute_worker_socket)) > 0) {
                    if(conn_socket > CONN_MAXFD) {
                        // TODO handle conn_socket fd exceed CONN_MAXFD
                        write_log(ERROR_LOG, "compute_worker_monitor: conn_socket exceed CONN_MAXFD\n");
                        close(conn_socket);
                    }
                    set_nonblocking(conn_socket);
                    //ev.data.fd = conn_socket;
                    //ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;

                    // save this connection into
                    conn = &(conn_mgr->conn_table[conn_socket]);
                    set_connection_info(conn, conn_socket, index, 1);
                    add_to_epoll(worker_epfd[index], conn_socket, EPOLLIN | EPOLLET | EPOLLONESHOT);

                    // here WORKER_NUM = (MANUAL_WORKER + FAST_WORKER + COMPUTE_WORKER)
                    index = (index + 1) % WORKER_NUM;
                    if(index == 0) {
                        index = MANUAL_WORKER + FAST_WORKER;
                    }
                }
            }
		} 
	}

    close(compute_worker_socket);
    close(epfd);
    pthread_cancel(pthread_self());
    return NULL;
}

void *worker(void *args){
    int epfd = *(int *)(((void**)args)[0]);
    connection_mgr_t conn_mgr = (connection_mgr_t)(((void**)args)[1]);

    int i, nfds;
    int conn_socket;
    connection_t conn;
	struct epoll_event events[MAX_EVENTS];

	while(conn_mgr->running) {
		nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);
		for(i = 0; i < nfds; i++) {
            conn_socket = events[i].data.fd;
            conn = &(conn_mgr->conn_table[conn_socket]);
            if (events[i].events && EPOLLIN) {
                if(FALSE == conn_mgr->read_event_func(conn)) {
                    close_connection(conn);
                }
                if(conn->index < (MANUAL_WORKER + FAST_WORKER)) {
                    if(conn->used != 0) {
                        modify_epoll(epfd, conn_socket, EPOLLIN | EPOLLONESHOT);
                    }
                }
			} else if(events[i].events && EPOLLOUT) {
                // only handle manual task (connection closed here)
                assert(conn->index < (MANUAL_WORKER + FAST_WORKER));
                if(FALSE == conn_mgr->write_event_func(conn)) {
                    close_connection(conn);
                }
                if(conn->used != 0) {
                    modify_epoll(epfd, conn_socket, EPOLLIN | EPOLLONESHOT);
                }

            } else if(events[i].events && EPOLLRDHUP) {
                close_connection(conn);
            } 

		}
	}
    pthread_cancel(pthread_self());
    return NULL;
}

void run_connection_mgr(connection_mgr_t conn_mgr) {
    // set running flag
    conn_mgr->running = 1;

    int i, j;
    //create EPOLL_NUM different epoll fd for workers
    for (i = 0; i < EPOLL_NUM; i++) {
        worker_epfd[i] = epoll_create(MAX_EVENTS);
    }

    /* used for pass worker_epfd and conn_mgr
     * to thread function worker
     * */
    void *args[EPOLL_NUM][2];
    // create worker threads to handle ready socket fds
    pthread_t worker_thread[WORKER_NUM];
    for(i = 0; i < EPOLL_NUM; i++) {
        for(j = 0; j < WORKER_PER_GROUP; j++) {
            args[i][0] = (void *)(worker_epfd + i);
            args[i][1] = (void *)conn_mgr; 
            pthread_create(worker_thread + (i * WORKER_PER_GROUP + j), NULL, worker, args[i]);
        }
    }
    
    // multiple threads in torus node
    pthread_create(&manual_worker_monitor_thread, NULL, manual_worker_monitor, conn_mgr);

    pthread_create(&fast_worker_monitor_thread, NULL, fast_worker_monitor, conn_mgr);

    pthread_create(&compute_worker_monitor_thread, NULL, compute_worker_monitor, conn_mgr);

    pthread_join(manual_worker_monitor_thread, NULL);
    pthread_join(fast_worker_monitor_thread, NULL);
    pthread_join(compute_worker_monitor_thread, NULL);
    for (i = 0; i < WORKER_NUM; i++) {
        pthread_join(worker_thread[i], NULL);
    }
}

void stop_connection_mgr(connection_mgr_t conn_mgr) {
    int i;

    // close worker epoll fds
    for (i = 0; i < EPOLL_NUM; i++) {
        close(worker_epfd[i]);
    }
    conn_mgr->running = 0;
}


//////////////////////////////////////////////////////////////////////

// put here temporarily

size_t read_message_size(const char * ptr_buf, size_t beg, size_t roff) {
    if(beg + sizeof(size_t) > roff) {
        return 0;
    }
    size_t msg_size;
    memcpy(&msg_size, (void *)(ptr_buf + beg), sizeof(size_t));
    return msg_size;
}

int handle_conn_buffer(connection_t conn) {
    struct message msg;
    size_t beg = 0;

    // msg_size = 0 means can't get complete message
    size_t msg_size = read_message_size(conn->rbuf, beg, conn->roff);
    if(msg_size > 0) {
        while(beg + msg_size <= conn->roff) {
            memcpy(&msg, conn->rbuf + beg, msg_size);
            beg = beg + msg_size;
            //process_message(conn, msg);
            message_handler* msg_h = find_message_handler(msg.op);
            if(msg_h != NULL) {
                msg_h->process2(msg);
            }

            msg_size = read_message_size(conn->rbuf, beg, conn->roff);
            if(msg_size == 0) {
                break;
            }
        }
    }
    size_t remained = conn->roff - beg;
    if( beg > 0) {
        memmove(conn->rbuf, conn->rbuf + beg, remained);
    }
    conn->roff = remained;
    return TRUE;
}

int handle_read_event(connection_t conn) {
    // special case when the connection is compute task
    if(conn->index >= MANUAL_WORKER + FAST_WORKER) {
        struct message msg;
        memset(&msg, 0, sizeof(struct message));

        if (TRUE == receive_message(conn->socketfd, &msg)) {
            //process_message(conn, msg);
            message_handler* msg_h = find_message_handler(msg.op);
            if(msg_h != NULL) {
                msg_h->process1(conn, msg);
            }
        } else {
            write_log(ERROR_LOG, "receive compute task request failed.\n");
            return FALSE;
        }
    } else {
        if(conn->roff == CONN_BUF_SIZE) {
            handle_conn_buffer(conn);
        }
        int ret = recv(conn->socketfd, conn->rbuf + conn->roff, CONN_BUF_SIZE - conn->roff, 0);
        if(ret > 0) {
            conn->roff += ret;
            handle_conn_buffer(conn);
        } else if(ret == 0){
            return FALSE;
        } else {
            if(errno != EINTR && errno != EAGAIN) {
                write_log(ERROR_LOG, "handle_read_event: socket error, %s\n", strerror(errno));
                return FALSE;
            }
            write_log(ERROR_LOG, "handle_read_event: error occerred, %s\n", strerror(errno));
        }
    }
    return TRUE;
}

int async_send_data(connection_t conn, const char * data, uint32_t data_len) {
    while(conn->woff + data_len > CONN_BUF_SIZE) {
        handle_write_event(conn);
    }
    memcpy(conn->wbuf + conn->woff, data, data_len);
    conn->woff += data_len;
    handle_write_event(conn);
    return TRUE;
}

int handle_write_event(connection_t conn) {
    if (conn->woff == 0) {
        return TRUE;
    }
    int ret = send(conn->socketfd, conn->wbuf, conn->woff, 0);
    if (ret < 0) {
        if (errno != EINTR && errno != EAGAIN) {
            write_log(ERROR_LOG, "handle_write_event: socket error, %s\n", strerror(errno));
            return FALSE;
        }
        write_log(ERROR_LOG, "handle_write_event: error occerred, %s\n", strerror(errno));
    } else {
        int remained = conn->woff - ret;
        if (remained > 0) {
            memmove(conn->wbuf, conn->wbuf + ret, remained);
        }
        conn->woff = remained;
    }
    return TRUE;
}



