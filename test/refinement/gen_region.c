#include<stdio.h>
#include<stdlib.h>
#include<time.h>

#include "gsl_rng.h"
#include "gsl_randist.h"

int main(int argc, char **argv) {
    double r_low[3], r_high[3];
    FILE *fout = fopen("./range_query", "w");
    int i, j;
    double low[3] = {39.85, 116.25, 39184.396898};
    double high[3] = {40.05, 116.5, 39413.969329};
    // init gsl lib
    gsl_rng *r;
    gsl_rng_env_setup();
    gsl_rng_default_seed = ((unsigned long)(time(NULL)));
    r = gsl_rng_alloc(gsl_rng_default);

    double x[100], y[100];
    for(i = 0; i < 100; i++) {
        x[i] = gsl_ran_flat(r, low[0], high[0] - 0.01);
        y[i] = gsl_ran_flat(r, low[1], high[1] - 0.01);
    }
    gsl_rng_free(r);

    int p = 100000;
    double m;
    int k = 0;
    while(p != 100) {
        for(k = 2; k <= 10; k += 2) { 
            m = k;
            for(i = 0; i < 100; i++) {
                r_low[0] = x[i];
                r_high[0] = r_low[0] + m / p;
                r_low[1] = y[i];
                r_high[1] = r_low[1] + m / p;
                r_low[2] = low[2];
                r_high[2] = high[2];
                fprintf(fout, "2 1 ");
                for(j = 0; j < 3; j++) {
                    fprintf(fout, "%lf %lf ", r_low[j], r_high[j]);
                }
                fprintf(fout, "\n");
            }
        }
        p = p / 10;
    }

    /*while(p != 100) {
        for(k = 2; k <= 10; k += 2) { 
            m = k;
            for(i = 0; i < 100; i++) {
                while(1) {
                    int p0 = rand() % p;
                    r_low[0] = low[0] + (high[0] - low[0]) * p0 / p;
                    r_high[0] = r_low[0] + m / p;
                    if(r_high[0] < high[0]) {
                        break;
                    }
                }
                while(1) {
                    int p1 = rand() % p;
                    r_low[1] = low[1] + (high[1] - low[1]) * p1 / p;
                    r_high[1] = r_low[1] + m / p;
                    if(r_high[1] < high[1]) {
                        break;
                    }
                }
                r_low[2] = low[2];
                r_high[2] = high[2];
                fprintf(fout, "2 1 ");
                for(j = 0; j < 3; j++) {
                    fprintf(fout, "%lf %lf ", r_low[j], r_high[j]);
                }
                fprintf(fout, "\n");
            }
        }
        p = p / 10;
    }*/
    fclose(fout);

    return 0;
}
