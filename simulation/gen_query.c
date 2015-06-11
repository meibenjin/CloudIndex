#include<stdio.h>
#include<stdlib.h>
#include<time.h>

#include "gsl_rng.h"
#include "gsl_randist.h"
#include "utils.h"

torus_partitions cluster_partitions[MAX_CLUSTERS_NUM];
int cluster_num;

int read_cluster_partitions() {
    FILE *fp;
    int count;
    fp = fopen(CLUSTER_PARTITONS, "rb");
    if(fp == NULL) {
        printf("read_cluster_partitons: open file %s failed.\n", CLUSTER_PARTITONS);
        return FALSE;
    }

    count = 0;
    torus_partitions tp;
	while (!feof(fp)) {
        fscanf(fp, "%d %d %d\n", &tp.p_x, &tp.p_y, &tp.p_z);
        cluster_partitions[count] = tp;
        count++;
    }
    cluster_num = count;
    fclose(fp);
    return TRUE;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("usage: %s cluster_id group_num.\n", argv[0]);
        exit(1);
    }
    double r_low[3], r_high[3];
    int i, j;

    // read cluster partitions from file 
    if(FALSE == read_cluster_partitions()) {
        exit(1);
    }
    printf("%d\n", cluster_num);
    
    //56980131.000000 58788063.000000 51682794.000000 53082437.000000 0.000000    5.000000
    double low[3] = {56977411.000000, 51683311.000000, 0.000000};
    double high[3] = {58790663.000000, 53082728.000000, 1064.000000};
    //double low[3] = {56980131.000000, 51682794.000000, 0.000000};
    //double high[3] = {58788063.000000, 53082437.000000, 5.000000};

    int cluster_id = atoi(argv[1]);
    int group_num = atoi(argv[2]);
    double range[3];

    //get torus partition

    range[0] = (high[0] - low[0]);
    range[1] = (high[1] - low[1]);
    range[2] = (high[2] - low[2]);

    low[2] += range[2] * cluster_id;
    high[2] += range[2] * cluster_id;

    // init gsl lib
    gsl_rng *r;
    gsl_rng_env_setup();
    gsl_rng_default_seed = ((unsigned long)(time(NULL)));
    r = gsl_rng_alloc(gsl_rng_default);

    double x_len, y_len, t_len;
    int k, count = 0, l;
    double p = 0.005;
    for(k = 2; k <= 10; k+=2){
        for(i = 0; i < group_num; i++) {
            int x;
            for(x = 4; x <= 4; x++) {
                r_low[0] = gsl_ran_flat(r, low[0], high[0] - range[0] * 0.05);
                r_low[1] = gsl_ran_flat(r, low[1], high[1] - range[1] * 0.05);
                r_low[2] = gsl_ran_flat(r, low[2], high[2] - range[2] * 0.05);

                for(l = 0; l < cluster_num; l++) {
                    torus_partitions tp = cluster_partitions[l];
                    char filename[20];
                    sprintf(filename, "./range_query_%d", tp.p_x * tp.p_y * tp.p_z);
                    FILE *fout = fopen(filename, "a+");
                    x_len = range[0] * p * k;
                    r_high[0] = r_low[0] + x_len;

                    y_len = range[1] * p * k;
                    r_high[1] = r_low[1] + y_len;

                    t_len = range[2] * p * k;
                    r_high[2] = r_low[2] + t_len;
                    fprintf(fout, "%d %d ", x, count);
                    for(j = 0; j < 3; j++) {
                        fprintf(fout, "%lf %lf ", r_low[j], r_high[j]);
                    }
                    fprintf(fout, "\n");
                    fclose(fout);
                }
                count++;
            }
        }
    }
    gsl_rng_free(r);


    return 0;
}
