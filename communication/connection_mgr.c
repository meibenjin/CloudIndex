/*
 * connection_mgr.c
 *
 *  Created on: Jan 16, 2015
 *      Author: meibenjin
 */

#include<stdlib.h>
#include<string.h>
#include<assert.h>
#include<sys/epoll.h>
#include<pthread.h>

#include "connection_mgr.h"
#include "logs/log.h"
#include "socket.h"

int worker_epfd[EPOLL_NUM];

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
        conn_mgr->conn_table[i].socketfd = 0;
        conn_mgr->conn_table[i].index = 0;
        conn_mgr->conn_table[i].used = 0;
        conn_mgr->conn_table[i].roff = 0;
        memset(conn_mgr->conn_table[i].rbuf, 0, CONN_BUF_SIZE);
        conn_mgr->conn_table[i].woff = 0;
        memset(conn_mgr->conn_table[i].wbuf, 0, CONN_BUF_SIZE);
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

void set_connection_info(connection_t conn, int conn_socket, int epoll_idx, int in_use) {
    conn->socketfd = conn_socket; 
    conn->index = epoll_idx; 
    conn->used = in_use;
}

void close_connection(connection_t conn) {
    struct epoll_event ev;
    conn->used = 0;
    conn->roff = 0;
    epoll_ctl(worker_epfd[conn->index], EPOLL_CTL_DEL, conn->socketfd, &ev);
    close(conn->socketfd);
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
    pthread_t manual_worker_monitor_thread;
    pthread_create(&manual_worker_monitor_thread, NULL, manual_worker_monitor, conn_mgr);

    pthread_t fast_worker_monitor_thread;
    pthread_create(&fast_worker_monitor_thread, NULL, fast_worker_monitor, conn_mgr);

    pthread_t compute_worker_monitor_thread;
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


