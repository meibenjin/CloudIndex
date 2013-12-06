/*
 * test.c
 *
 *  Created on: Sep 17, 2013
 *      Author: meibenjin
 */

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>

void gen_range(int min, int max, float p) {
	int low, high, step, i, randint;

    step = max * p;
    max = max - max * p;

	for (i = 0; i < 3; i++) {
		randint = rand();
		low = (randint % (max - min)) + min;
		high = low + step - 1 ;
		printf("%d %d ", low, high);
	}
	printf("\n");
}

int main(void) {
	int i;
    srand(time(NULL));
	for(i = 0; i< 100;i++){
		gen_range(1, 100, 0.4);
	}


	return 0;
}






















