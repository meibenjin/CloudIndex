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

clock_t sample_gen;
double sample_el = 0;

//return elasped time from start to finish
long get_elasped_time(struct timespec start, struct timespec end) {
	return 1000000000L * (end.tv_sec - start.tv_sec)
			+ (end.tv_nsec - start.tv_nsec);
}

int point_contain(point p, interval o[]) {
	int i, ovlp = 1;
	i = 0;
	while (ovlp && i < MAX_DIM_NUM) {
		ovlp = !(p.axis[i] < o[i].low || p.axis[i] > o[i].high);
		i++;
	}
    return ovlp;
}

double calc_QP(struct interval region[], point start, point end, int n, int m, point **samples) {
    int cnt, i, j;
    double value, z;
    // center_point
    struct point cpt;

    // init gsl lib
    gsl_rng *r;
    gsl_rng_env_setup();
    gsl_rng_default_seed = ((unsigned long)(time(NULL)));
    r = gsl_rng_alloc(gsl_rng_default);

    sample_gen = clock();
    for(i = 0, cnt = 0; cnt < n; cnt++) {
        // sample z(time) axis with uniform distribution
        value = gsl_ran_flat(r, start.axis[2], end.axis[2]);

        // sample x, y axis with gaussian distribution
        /* given points start and end and sample point p's z axis,
         * note that these three points are in one line.
         * calc samples point's x, y method:
         *  (p.x[y] - start.x[y])     (end.x[y] - start.x[y]) 
         * ----------------------- = -------------------------
         *    (p.z - start.z)              (end.z - p.z) 
         * */
        z = value;
        cpt.axis[0] = ((z - start.axis[2]) * end.axis[0] + (end.axis[2] - z) * start.axis[0]) / (end.axis[2] - start.axis[2]);
        cpt.axis[1] = ((z - start.axis[2]) * end.axis[1] + (end.axis[2] - z) * start.axis[1]) / (end.axis[2] - start.axis[2]);
        cpt.axis[2] = z;

        // if one of n flat distribution sample point not contianed in region, 
        // skip this sample point
        if(1 == point_contain(cpt, region)) {
            for(j = 0; j < m; j++) {
                samples[i][j].axis[2] = cpt.axis[2];
            }

            j = 0;
            while(j < m) {
                value = gsl_ran_gaussian(r, SIGMA); 
                if(value < -3 * SIGMA || value > 3 * SIGMA) {
                    continue;
                }
                samples[i][j].axis[0] = cpt.axis[0] + value;
                j++;
            }

            j = 0;
            while(j < m) {
                value = gsl_ran_gaussian(r, SIGMA); 
                if(value < -3 * SIGMA || value > 3 * SIGMA) {
                    continue;
                }
                samples[i][j].axis[1] = cpt.axis[1] + value;
                j++;
            }
            i++;
        }
    }
    sample_el += (double)(clock()- sample_gen) / CLOCKS_PER_SEC;
    gsl_rng_free(r);

    // i is the final flat sample points
    int flat_cnt = i;
    // calculate QP
    double qp = 0.0, F[n], MF = 1;
    // the probability of each time samples(n)
    double pt = 1.0 / m;
    for(i = 0; i < flat_cnt; i++) {
        F[i] = 0.0;
        for(j = 0; j < m; j++) {
            if(1 == point_contain(samples[i][j], region)) {
                F[i] += pt;
            }
        }
        MF *= (1 - F[i]);
    }
    qp = 1 - MF;
    return qp;
}

double calc_refinement(struct interval region[], point start, point end) {
    // the time sampling size n 
    // and other two sampling size m
    int i, j, k, n = SAMPLE_TIME_POINTS, m = SAMPLE_SPATIAL_POINTS;
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

    // generate n*m samples points
    double qp = calc_QP(region, start, end, n, m, samples);

    //printf("[%lf,%lf] [%lf, %lf] [%lf, %lf]", region[0].low, region[0].high, region[1].low, region[1].high,region[2].low, region[2].high);
    //printf("(%lf,%lf,%lf) ", start.axis[0], start.axis[1], start.axis[2]);
    //printf("(%lf,%lf,%lf)\n", end.axis[0], end.axis[1], end.axis[2]);

    // release the samples mem
    for(i = 0; i < n; i++){
        free(samples[i]);
    }
    free(samples);

    return qp;
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


double points_distance(struct point p1, struct point p2) {
    double distance = 0;
    int i;
    for(i = 0; i < MAX_DIM_NUM; i++) {
        distance += (p1.axis[i] - p2.axis[i]) * (p1.axis[i] - p2.axis[i]); 
    }
    return sqrt(distance);
}

int Comp(const void *p1,const void *p2, void *p0){
    point qpt = *(point *)p0;
    double dis1 = points_distance((*(point *)p1), qpt);
    double dis2 = points_distance((*(point *)p2), qpt);
    if(dis1 < dis2) {
        return -1;
    } else {
        return 1;
    }
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
    //sort_by_dis(n, m, samples);
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

double eval_range_refinement_once(int seg_num){
    double elapse = 0;
    //region is big enough;
    struct interval region[MAX_DIM_NUM];
    region[0].low = 56980131.000000;
    region[0].high = 58788063.000000;
    region[1].low = 51682794.000000;
    region[1].high = 53082437.000000;
    region[2].low = 0;
    region[2].high = 5;

    double low[3] = {56980131.000000, 51682794.000000, 0.000000};
    double high[3] = {58788063.000000, 53082437.000000, 5.000000};

    // init gsl lib
    gsl_rng *r;
    gsl_rng_env_setup();
    gsl_rng_default_seed = ((unsigned long)(time(NULL)));
    r = gsl_rng_alloc(gsl_rng_default);

    int i;
    for (i=0;i<seg_num;i++){
        point start_pt, end_pt;
        start_pt.axis[0] = gsl_ran_flat(r, low[0], high[0]);
        start_pt.axis[1] = gsl_ran_flat(r, low[1], high[1]);
        start_pt.axis[2] = gsl_ran_flat(r, low[2], high[2]);
        end_pt.axis[0] = start_pt.axis[0] + 4000;
        end_pt.axis[1] = start_pt.axis[1] + 4000;
        end_pt.axis[2] = start_pt.axis[2] + 1;
        //random generate a line segment with fixed width;
        //random generate a point as the start;
        // for the end, in time axis, start+the sampling rate;  in spatial domain, start+max_speed;
        clock_t start = clock();
        calc_refinement(region, start_pt, end_pt);
        elapse += (double)(clock()-start) / CLOCKS_PER_SEC;
    }
    gsl_rng_free(r);
    return elapse;
}
void eval_range_refinement_efficiency(){
    int i;
    int seg_num;
    for (seg_num = 100; seg_num <= 1000; seg_num += 100){
        double total_elapse = 0;
        for (i=0;i<10;i++){
            total_elapse += eval_range_refinement_once(seg_num);
        }
        printf("%d, %lf, %lf\n", seg_num, sample_el/10.0, total_elapse/10.0);
        sample_el = 0;
    }
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
    eval_range_refinement_efficiency();
    return 0;
}


