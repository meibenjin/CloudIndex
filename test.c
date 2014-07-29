/*
 * test.c
 *
 *  Created on: Sep 17, 2013
 *      Author: meibenjin
 */

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/time.h>
#include<time.h>
#include<unistd.h>
#include<math.h>
#include<sys/epoll.h>
#include<pthread.h>
#include<errno.h>

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

    char file[MAX_FILE_NAME];
    snprintf(file, MAX_FILE_NAME, "%s/%s", DATA_DIR, file_name);
	fp = fopen(file, "rb");
	if (fp == NULL) {
		printf("can't open file %s\n", file);
		exit(1);
	}

    // create socket to server for transmitting data points
    int socketfd;
    socketfd = new_client_socket(entry_ip, MANUAL_WORKER_PORT);
    if (FALSE == socketfd) {
        return FALSE;
    }

    // get local ip
	char local_ip[IP_ADDR_LENGTH];
	memset(local_ip, 0, IP_ADDR_LENGTH);
	if (FALSE == get_local_ip(local_ip)) {
        close(socketfd);
		return FALSE;
	}

    // basic packet to be sent
    struct message msg;
    msg.op = QUERY_TORUS_CLUSTER;
	strncpy(msg.src_ip, local_ip, IP_ADDR_LENGTH);
	strncpy(msg.dst_ip, entry_ip, IP_ADDR_LENGTH);
	strncpy(msg.stamp, "", STAMP_SIZE);

    size_t cpy_len = 0; 
	int hops= 0;

    if(strcmp(file_name, "range_query") == 0){
        printf("begin range query.\n");
        while (!feof(fp)) {
            fscanf(fp, "%d %d", &query.op, &query.data_id);
            //printf("%d. ", ++count);
            for (i = 0; i < MAX_DIM_NUM; i++) {
                fscanf(fp, "%lf %lf ", &query.intval[i].low, &query.intval[i].high);
            }
            fscanf(fp, "\n");
            /*printf("%d %d %lf %lf %lf %lf %lf %lf\n", query.op, query.data_id, \
                    query.intval[0].low, query.intval[0].high, \
                    query.intval[1].low, query.intval[1].high,\
                    query.intval[2].low, query.intval[2].high);*/

            // package query into message struct
            cpy_len = 0;
            memset(msg.data, 0, DATA_SIZE);
            memcpy(msg.data, &hops, sizeof(int));
            cpy_len += sizeof(int);
            memcpy(msg.data + cpy_len, (void *)&query, sizeof(struct query_struct));
            cpy_len += sizeof(struct query_struct);
            send_safe(socketfd, (void *) &msg, sizeof(struct message), 0);
            if(count % 100 == 0) {
                printf("%d\n", count);
            }
            count++;
            usleep(5000);
        }
        printf("finish range query.\n");
        fclose(fp);
    }
    close(socketfd);

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
            sleep(2);
        }
        printf("finish nn query.\n");
        fclose(fp);
    }

    return TRUE;
}

double points_distance(struct point p1, struct point p2) {
    double distance = 0;
    int i;
    for(i = 0; i < MAX_DIM_NUM; i++) {
        distance += (p1.axis[i] - p2.axis[i]) * (p1.axis[i] - p2.axis[i]); 
    }
    return sqrt(distance);
}

int Comp(const void *p1,const void *p2){
    int i;
    point qpt;
    for(i = 0; i < MAX_DIM_NUM; i++) {
        qpt.axis[i] = 0;
    }
    double dis1 = points_distance((*(point *)p2), qpt);
    double dis2 = points_distance((*(point *)p1), qpt);
    return dis1 < dis2;
}

void sort_by_dis(int n, int m, point **samples) {
    qsort(samples[0], m, sizeof(samples[0][0]), Comp);
}

int evaluate_abg(double  * alpha, double * beta, double * gama) {
    clock_t s, e;
    s = clock(); 

    int i, j, k, n = 1, m = 10000;
    struct point start, end;
    for(k = 0; k < MAX_DIM_NUM; k++) {
        start.axis[k] = 1;
        end.axis[k] = 1000;
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
    s = clock();
    gen_sample_point(start, end, n, m, samples);
    e = clock();

    *alpha =  (double)(e - s) / CLOCKS_PER_SEC; 

    s = clock(); 
    //sort 
    sort_by_dis(n, m, samples);
    e = clock();

    *beta =  (double)(e - s) / CLOCKS_PER_SEC; 
  
    s = clock();
    // for i = n*m 
    //  for i = n*m 
    //      m = m * a; 
    double mul = 1.0;
    for(i = 0; i < m; i++) {
        for(j = 0; j < m; j++){
            mul *= samples[0][j].axis[0]; 
        }
    }
    e = clock();
    *gama =  (double)(e - s) / CLOCKS_PER_SEC; 
    
    e = clock(); 
    for(i = 0; i < n; i++){
        free(samples[i]);
    }
    free(samples);
	return 0;
}

int main(int argc, char **argv){
    /*double alpha, beta, gama; 
    double a, b, g;
    a = b =g =0;
    int i;
    for (i=0;i<10;i++){
       evaluate_abg(&alpha, &beta, &gama);
       a += alpha;
       b += beta;
       g += gama;
    }
    printf("avg alpha=%lf, avg beta = %lf, avg gama=%lf\n",a,b,g);*/
    range_query(argv[1], argv[2]);
    return 0;
}




