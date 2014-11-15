/*
 * cqueue.c
 *
 *  Created on: Sep 24, 2013
 *      Author: meibenjin
 */

#include<string.h>

#include"cqueue.h"

int init_queue(struct cqueue *q) {
    memset(q->name, 0, IP_ADDR_LENGTH);
    memset(q->data, 0, sizeof(query_struct) * MAX_CQUEUE_SIZE);
    q->front = 0;
    q->rear = 0;
    return TRUE;
}

int full_queue(struct cqueue *q) {
    if(q->front == (q->rear + 1) % MAX_CQUEUE_SIZE){
        return TRUE;
    } else {
        return FALSE;
    }
}

int empty_queue(struct cqueue *q) {
    if(q->front == q->rear) {
        return TRUE;
    } else {
        return FALSE;
    }
}
int len_queue(struct cqueue *q) {
    int len = (q->rear - q->front + MAX_CQUEUE_SIZE) % MAX_CQUEUE_SIZE;
    return len; 
}

int enter_queue(struct cqueue *q, struct query_struct item) {
    if(TRUE == full_queue(q)) {
        return FALSE;
    } else {
        q->data[q->rear] = item;
        q->rear = (q->rear + 1) % MAX_CQUEUE_SIZE;
        return TRUE;
    }
}

int dequeue(struct cqueue *q, struct query_struct *item) {
    if(TRUE == empty_queue(q)) {
        return FALSE;
    } else {
        *item = q->data[q->front];
        q->front = (q->front + 1) % MAX_CQUEUE_SIZE;
        return TRUE;
    }
}

