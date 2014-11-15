#include<stdio.h>
#include<stdlib.h>
#include<time.h>

#include "gsl_rng.h"
#include "gsl_randist.h"

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("usage: %s cluster_id group_num.\n", argv[0]);
        exit(1);
    }
    double r_low[3], r_high[3];
    FILE *fout = fopen("./range_query", "w");
    int i, j;
    
    //56977411.000000	58790663.000000	51682794.000000	53082728.000000	0.000000	5000.000000
    //56977412.648099   58790659.853901 51682794.000000 53082726.952503 1.000000    3622.333333
    double low[3] = {56977412.648099, 51682794.000000, 0.000000};
    double high[3] = {58790659.853901, 53082726.952503, 3622.333333};
    
    //56980131.000000 58788063.000000 51682794.000000 53082437.000000 0.000000    5.000000
    //double low[3] = {56980131.000000, 51682794.000000, 0.000000};
    //double high[3] = {58788063.000000, 53082437.000000, 5.000000};

    int cluster_id = atoi(argv[1]);
    int group_num = atoi(argv[2]);
    double range[3];
    for(i = 0; i < 3; i++) {
        range[i] = high[i] - low[i];
    }

    low[2] += range[2] * cluster_id;
    high[2] += range[2] * cluster_id;

    // init gsl lib
    gsl_rng *r;
    gsl_rng_env_setup();
    gsl_rng_default_seed = ((unsigned long)(time(NULL)));
    r = gsl_rng_alloc(gsl_rng_default);

    double x_len, y_len, t_len;
    int k, count = 0;
    double p = 0.005;
    for(k = 2; k <= 10; k+=2){
        for(i = 0; i < group_num; i++) {
            r_low[0] = gsl_ran_flat(r, low[0], high[0] - range[0] * 0.05);
            x_len = range[0] * p * k;
            r_high[0] = r_low[0] + x_len;

            r_low[1] = gsl_ran_flat(r, low[1], high[1] - range[1] * 0.05);
            y_len = range[1] * p * k;
            r_high[1] = r_low[1] + y_len;

            r_low[2] = gsl_ran_flat(r, low[2], high[2] - range[2] * 0.05);
            t_len = range[2] * p * k;
            r_high[2] = r_low[2] + t_len;

            count++;
            // the first 2 means range query operate code
            fprintf(fout, "3 %d ", count);
            for(j = 0; j < 3; j++) {
                fprintf(fout, "%lf %lf ", r_low[j], r_high[j]);
            }
            fprintf(fout, "\n");
        }
    }
    gsl_rng_free(r);

    fclose(fout);

    return 0;
}
