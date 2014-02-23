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


int main(void) {
	int i;
    srand(time(NULL));
	for(i = 0; i< 10000;i++){
		gen_range(1, 100, 0.4);
	}

    performance_test("172.16.0.212");

	/*struct timespec query_start, query_end;
    double start_time, end_time = 0.0;
    int i;
	clock_gettime(CLOCK_REALTIME, &query_start);
    start_time = (double)(1000000000L * query_start.tv_sec + query_start.tv_nsec);
    for(i = 0; i < 1000; i++) {
        FILE *fp = fopen("./test", "a+");
        if (fp == NULL) {
            printf("can't open file test\n");
            exit(0);
        }
        fprintf(fp, "start query time:%f ns\n\n", start_time);
        fclose(fp);
    }
	clock_gettime(CLOCK_REALTIME, &query_end);
    end_time = (double)(1000000000L * query_end.tv_sec + query_end.tv_nsec);
    for(i = 0; i < 10000; i++) {
        FILE *fp = fopen("./test", "a+");
        if (fp == NULL) {
            printf("can't open file test\n");
            exit(0);
        }
        fprintf(fp, "send query time:%f ns\n\n", end_time);
        fclose(fp);
    }*/

	/*FILE *fp = fopen("./range_query", "rb");
    int count = 0;
    int a, b, c, d, e, f;
    int g, h ;
    while(!feof(fp)){
        fscanf(fp, "%d %d", &g, &h);
        fscanf(fp, "%d %d %d %d %d %d", &a, &b, &c, &d, &e, &f);
        count++;
    }
    fclose(fp);
    printf("%d\n", count);*/
	return 0;
}




