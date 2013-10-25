/*
 * test.c
 *
 *  Created on: Sep 17, 2013
 *      Author: meibenjin
 */

#include<stdio.h>
#include<stdlib.h>
#include<string.h>

struct A{
    int a;
    char b;
};

struct B{
	int a;
	struct A n[];
};
struct B *new_B(){
    struct A t = { 100, 'c'};
    struct B *p;
    p = (struct B *) malloc(sizeof(struct B) + sizeof(struct A));
    p->a = 1;
    //p->n = (struct A *) malloc(sizeof(struct A));
    memcpy(&p->n[0], (void *)&t, sizeof(struct A));
    return p;
}
int main(void) {
    struct B *p;
    p = new_B();
    printf("%d, %d, %c\n", p->a, p->n->a, p->n->b);
    free(p);
    return 0;
}

