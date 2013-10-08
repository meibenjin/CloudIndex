/*
 * test.c
 *
 *  Created on: Sep 17, 2013
 *      Author: meibenjin
 */

#include<stdio.h>
#include<stdlib.h>
#include<string.h>

int main(void) {
	char test[40];
	memset(test, 0, 40);
	printf("%d\n", strcmp(test, ""));
}

