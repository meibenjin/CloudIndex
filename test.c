/*
 * test.c
 *
 *  Created on: Sep 17, 2013
 *      Author: meibenjin
 */

#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include"utils.h"

int overlaps(interval c[], interval o[]) {
    int i, ovlp = 1;
    i = -1;
    do {
        i++;
        ovlp = !(c[i].high < o[i].low || c[i].low > o[i].high); 
    }while(ovlp && i != MAX_DIM_NUM);

    return ovlp;
}

int main(void) {
    interval c[MAX_DIM_NUM] = {{40, 80}, { 100, 120 }, {100, 120}};
    interval o[MAX_DIM_NUM] = {{74, 81}, { 113, 118 }, {96, 105}};
    printf("%d\n", overlaps(c, o));
    return 0;
}

