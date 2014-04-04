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

#include "utils.h"
#include "socket/socket.h"

int query_test(struct query_struct query, const char *entry_ip) {

	// get local ip address
	char local_ip[IP_ADDR_LENGTH];
	memset(local_ip, 0, IP_ADDR_LENGTH);
	if (FALSE == get_local_ip(local_ip)) {
		return FALSE;
	}

	struct message msg;
	msg.op = QUERY_TORUS_CLUSTER;
	strncpy(msg.src_ip, local_ip, IP_ADDR_LENGTH);
	strncpy(msg.dst_ip, entry_ip, IP_ADDR_LENGTH);
	strncpy(msg.stamp, "", STAMP_SIZE);

    size_t cpy_len = 0; 
	int count = 0;
	memcpy(msg.data, &count, sizeof(int));
    cpy_len += sizeof(int);

    memcpy(msg.data + cpy_len, (void *)&query, sizeof(struct query_struct));
    cpy_len += sizeof(struct query_struct);

    if(FALSE == forward_message(msg, 0)) {
        return FALSE;
    }

	return TRUE;
}

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
	/*int i;
    srand(time(NULL));
	for(i = 0; i< 10000;i++){
		gen_range(1, 100, 0.4);
	}*/

	int count = 0, i;
    struct query_struct query;
    FILE *fp;

    char data_file[MAX_FILE_NAME];
    snprintf(data_file, MAX_FILE_NAME, "%s/data", DATA_DIR);
	fp = fopen(data_file, "rb");
	if (fp == NULL) {
		printf("can't open file\n");
		exit(1);
	}

    printf("begin read.\n");
    //1 1   39.984702   116.318417  39744.120185    1
    //1 2   39.984683   116.318450  39744.120255    1
	while (!feof(fp)) {

        fscanf(fp, "%d\t%d\t", &query.op, &query.data_id);
        //printf("%d ", query.data_id);
        count++;
        for (i = 0; i < MAX_DIM_NUM; i++) {
            #ifdef INT_DATA
                fscanf(fp, "%d", &query.intval[i].low);
                //printf("%d ", query.intval[i].low);
            #else
                double value;
                fscanf(fp, "%lf", &value);
                query.intval[i].low = value;
                query.intval[i].high = value;
                //printf("%lf ", query.intval[i].low);
            #endif
        }
        fscanf(fp, "%d", &query.trajectory_id);
        //printf("%d ", query.trajectory_id);

        fscanf(fp, "\n");
        //printf("\n");

        if( FALSE == query_test(query, "172.16.0.166")) {
            printf("%d\n", count);
            break;
        }
        if(count % 1000 == 0) {
            printf("%d\n", count);
        }
	}
    printf("finish read.\n");
	fclose(fp);
    //performance_test("172.16.0.166");

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




