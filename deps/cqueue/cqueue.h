/*
 * cqueue.h
 *
 *  Created on: Nov 14, 2014
 *      Author: meibenjin
 */

#ifndef CQUEUE_H_
#define CQUEUE_H_

#include"utils.h"

#define MAX_CQUEUE_SIZE 60000

typedef struct cqueue{
    char name[IP_ADDR_LENGTH];
    struct query_struct data[MAX_CQUEUE_SIZE];
    int front, rear; 
}cqueue;

int init_queue(struct cqueue *q);

int full_queue(struct cqueue *q);

int empty_queue(struct cqueue *q);

int len_queue(struct cqueue *q);

int enter_queue(struct cqueue *q, struct query_struct item);

int dequeue(struct cqueue *q, struct query_struct *item);


#endif /* CQUEUE_H_ */
