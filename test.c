/*
 * test.c
 *
 *  Created on: Sep 17, 2013
 *      Author: meibenjin
 */

#include<stdio.h>
#include<stdlib.h>
#include<string.h>

void show_bytes(char *p, int size) {
    int i;
    for(i = 0; i < size; i++){
        printf("0x%.2x ", *p++);
    }
    printf("\n");
}

int main(void) {
    int a = 0x01;
    int l = htonl(a);
    printf("%d:", a);
    show_bytes((void *)&a, sizeof(int));
    printf("%d:", l);
    show_bytes((void *)&l, sizeof(int));
    return 0;
}

