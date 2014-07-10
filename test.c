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
#include "control_node/control.h"

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

/*int gen_sample_point(point start, point end, int n, int m, point **samples) {
    int i, j;
    double value, x, y;

    gsl_rng *r;
    gsl_rng_env_setup();
    gsl_rng_default_seed = ((unsigned long)(time(NULL)));
    r = gsl_rng_alloc(gsl_rng_default);

    for(i = 0; i < n; i++) {
        if(i == 0) {
            value = start.z;
        } else if(i == n - 1) {
            value = end.z;
        } else {
            // sample z(time) axis with uniform distribution
            value = gsl_ran_flat(r, start.z, end.z);
        }

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
}*/

int gen_sample_point(point start, point end, int n, int m, point **samples) {
    int i, j;
    double value, x, y;

    gsl_rng *r;
    gsl_rng_env_setup();
    gsl_rng_default_seed = ((unsigned long)(time(NULL)));
    r = gsl_rng_alloc(gsl_rng_default);
    
    for(i = 0; i < n; i++) {
        if(i == 0) {
            value = start.axis[2];
        } else if(i == n - 1) {
            value = end.axis[2];
        } else {
            // sample z(time) axis with uniform distribution
            value = gsl_ran_flat(r, start.axis[2], end.axis[2]);
        }

        for(j = 0; j < m; j++) {
            samples[i][j].axis[2] = value;
        }

        // sample x, y axis with gaussian distribution
        /* given points start and end and sample point p's z axis,
         * note that these three points are in one line.
         * calc samples point's x, y method:
         *  (p.x[y] - start.x[y])     (end.x[y] - start.x[y]) 
         * ----------------------- = -------------------------
         *    (p.z - start.z)              (end.z - p.z) 
         * */

        j = 0;
        while(j < m) {
            value = gsl_ran_gaussian(r, SIGMA); 
            if(value < -3 * SIGMA || value > 3 * SIGMA) {
                continue;
            }
            x = ((samples[i][j].axis[2] - start.axis[2]) * end.axis[0] + (end.axis[2] - samples[i][j].axis[2]) * start.axis[0]) / (end.axis[2] - start.axis[2]);
            samples[i][j].axis[0] = x + value;
            j++;
        }

        j = 0;
        while(j < m) {
            value = gsl_ran_gaussian(r, SIGMA); 
            if(value < -3 * SIGMA || value > 3 * SIGMA) {
                continue;
            }
            y = ((samples[i][j].axis[2] - start.axis[2]) * end.axis[1] + (end.axis[2] - samples[i][j].axis[2]) * start.axis[1]) / (end.axis[2] - start.axis[2]);
            samples[i][j].axis[1] = y + value;
            j++;
        }
    }

    gsl_rng_free(r);
    return TRUE;
}

double calc_Qp(struct interval region[], int n, int m, point **samples) {
    struct interval intval[MAX_DIM_NUM];
    int i, j, k;
    double qp = 0.0, F[n], MF = 1;
    // the probability of each time samples(n)
    //double pt = 1.0 / n;

    for(i = 0; i < n; i++) {
        F[i] = 0.0;
        for(j = 0; j < m; j++) {
            for(k = 0; k < MAX_DIM_NUM; k++) {
                intval[k].low = intval[k].high = samples[i][j].axis[k];
            }
            //if(1 == overlaps(intval, region)) {
            //    F[i] += pt;
            //}
        }
        MF *= (1 - F[i]);
    }
    qp = 1 - MF;

    return qp;
}

int query_oct_tree(struct query_struct query, const char *entry_ip) {

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

int range_query(const char *entry_ip, char *file_name) {
	int count = 0, i;
    struct query_struct query;
    FILE *fp;

    char file[20];
    snprintf(file, 20, "./%s", file_name);
	fp = fopen(file, "rb");
	if (fp == NULL) {
		printf("can't open file %s\n", file);
		exit(1);
	}

    if(strcmp(file_name, "range_query") == 0){
        printf("begin range query.\n");
        while (!feof(fp)) {
            fscanf(fp, "%d %d", &query.op, &query.data_id);
            //printf("%d. ", ++count);
            for (i = 0; i < MAX_DIM_NUM; i++) {
                fscanf(fp, "%lf %lf ", &query.intval[i].low, &query.intval[i].high);
            }
            printf("X:[%lf,%lf] ", query.intval[0].low, query.intval[0].high);
            printf("Y:[%lf,%lf] ", query.intval[1].low, query.intval[1].high);
            printf("T:[%lf,%lf] ", query.intval[2].low, query.intval[2].high);
            fscanf(fp, "\n");
            printf("\n");

            if(FALSE == query_oct_tree(query, entry_ip)) {
                printf("%d\n", count);
                break;
            }
            printf("\n");
            usleep(2000);
        }
        printf("finish range query.\n");
        fclose(fp);
    }

    if(strcmp(file_name, "nn_query") == 0){
        printf("begin nn query.\n");
        while (!feof(fp)) {
            fscanf(fp, "%d %d", &query.op, &query.data_id);
            //printf("%d. ", ++count);
            for (i = 0; i < MAX_DIM_NUM; i++) {
                fscanf(fp, "%lf %lf ", &query.intval[i].low, &query.intval[i].high);
            }
            printf("x:%lf ", query.intval[0].low);
            printf("y:%lf ", query.intval[1].low);
            printf("T:[%lf,%lf]", query.intval[2].low, query.intval[2].high);
            fscanf(fp, "\n");
            printf("\n");

            if(FALSE == query_oct_tree(query, entry_ip)) {
                printf("%d\n", count);
                break;
            }
            printf("\n");
            usleep(2000);
        }
        printf("finish nn query.\n");
        fclose(fp);
    }

    return TRUE;
}

int main(int argc, char **argv) {

    /*int i, j, k, n = 5, m = 5;
    struct point start, end;
    for(k = 0; k < MAX_DIM_NUM; k++) {
        start.axis[k] = 1;
        end.axis[k] = 10;
    }
    struct point **samples;
    samples = (point **)malloc(sizeof(point*) * n);
    for(i = 0; i < n; i++) {
        samples[i] = (point *)malloc(sizeof(point) * m);
        for(j = 0; j < m; j++) {
            for(k = 0; k < MAX_DIM_NUM; k++) {
                samples[i][j].axis[k] = 0;
            }
        }
    } 

    gen_sample_point(start, end, n, m, samples);

    for(i = 0; i < n; i++) {
        for(j = 0; j < m; j++) {
            printf("[%.2lf,%.2lf,%.2lf] ", samples[i][j].axis[0], samples[i][j].axis[1], samples[i][j].axis[2]);
        }
        printf("\n");
    }
    printf("\n");

    for(i = 0; i < n; i++){
        free(samples[i]);
    }
    free(samples);*/


	/*int i;
    srand(time(NULL));
	for(i = 0; i< 10000;i++){
		gen_range(1, 100, 0.4);
	}*/

    range_query("172.16.0.20", argv[1]);
    //performance_test("172.16.0.212");
    //performance_test(argv[1]);
	return 0;
}




