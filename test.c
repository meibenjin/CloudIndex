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
#include<sys/time.h>

#include"utils.h"
#include"socket/socket.h"

void gen_range(int min, int max, float p) {
	int low, high, step, i, randint;

    step = max * p;
    max = max - max * p;

    printf("%d %d ", 2, 1);
    int rlow[3], rhigh[3];
	for (i = 0; i < 3; i++) {
		randint = rand();
		low = (randint % (max - min)) + min;
		high = low + step - 1 ;
        rlow[i] = low;
        rhigh[i] = high;
	}
	for (i = 0; i < 3; i++)
        printf("%d ", rlow[i]);
	for (i = 0; i < 3; i++) 
        printf("%d ", rhigh[i]);
	printf("\n");
}


//return elasped time from start to finish
long get_elasped_time(struct timespec start, struct timespec end) {
	return 1000000000L * (end.tv_sec - start.tv_sec)
			+ (end.tv_nsec - start.tv_nsec);
}


int main(int argc, char **argv) {
	/*int i;
    srand(time(NULL));
	for(i = 0; i< 10000;i++){
		gen_range(1, 100, 0.4);
	}*/

    performance_test(argv[1]);

	return 0;
}




