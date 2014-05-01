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

#include "gsl_rng.h"
#include "gsl_randist.h"

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

int gen_sample_point(point start, point end, int n, int m, point **samples) {
    int i, j;
    double value, x, y;

    gsl_rng *r;
    gsl_rng_env_setup();
    gsl_rng_default_seed = ((unsigned long)(time(NULL)));
    r = gsl_rng_alloc(gsl_rng_default);

    for(i = 0; i < n; i++) {
        // sample z(time) axis with uniform distribution
        value = gsl_ran_flat(r, start.z, end.z);
        for(j = 0; j < m; j++) {
           samples[i][j].z = value;
        }

        // sample x, y axis with gaussian distribution
        j = 0;
        while(j < m) {
           value = gsl_ran_gaussian(r, SIGMA); 
           if(value < -3 * SIGMA || value > 3 * SIGMA) {
               continue;
           }
           x = ((samples[i][j].z - start.z) * end.x + (end.z - samples[i][j].z) * start.x) / (end.z - start.z);
           samples[i][j].x = x + value;
           j++;
        }
        j = 0;

        while(j < m) {
           value = gsl_ran_gaussian(r, SIGMA); 
           if(value < -3 * SIGMA || value > 3 * SIGMA) {
               continue;
           }
           y = ((samples[i][j].z - start.z) * end.y + (end.z - samples[i][j].z) * start.y) / (end.z - start.z);
           samples[i][j].y = y + value;
           j++;
        }
    }

    gsl_rng_free(r);
    return TRUE;
}

int main(int argc, char **argv) {

    int i, j, n = 5, m = 5;
    struct point start, end;
    start.x = start.y = start.z = 1;
    end.x = end.y = end.z = 10;
    struct point **samples;
    samples = (point **)malloc(sizeof(point*) * n);
    for(i = 0; i < n; i++) {
        samples[i] = (point *)malloc(sizeof(point) * m);
        for(j = 0; j < m; j++) {
            samples[i][j].x = 0;
            samples[i][j].y = 0;
            samples[i][j].z = 0;
        }
    } 

    gen_sample_point(start, end, n, m, samples);

    for(i = 0; i < n; i++) {
        for(j = 0; j < m; j++) {
            printf("[%.2lf,%.2lf,%.2lf] ", samples[i][j].x, samples[i][j].y, samples[i][j].z);
        }
        printf("\n");
    }
    printf("\n");

    for(i = 0; i < n; i++){
        free(samples[i]);
    }


	/*int i;
    srand(time(NULL));
	for(i = 0; i< 10000;i++){
		gen_range(1, 100, 0.4);
	}*/

	/*int count = 0, i;
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
	fclose(fp);*/
    //performance_test("172.16.0.166");
    //performance_test(argv[1]);
	return 0;
}




