/*
 * geometry.h
 *
 *  Created on: Jan 15, 2015
 *      Author: meibenjin
 */


#include<math.h>
#include "geometry.h"

int overlaps(interval c[], interval o[]) {
	int i, ovlp = 1;
	i = 0;
	while (ovlp && i < MAX_DIM_NUM) {
		ovlp = !(c[i].high < o[i].low || c[i].low > o[i].high);
		i++;
	}

	return ovlp;
}

int interval_overlap(interval cinterval, interval ointerval) {
    if(cinterval.high < ointerval.low){
        return -1;
    }

    if (cinterval.low > ointerval.high) {
        return 1;
    }

    return 0;
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

int line_intersect(interval o[], point start, point end) {
    int intersect = 1;
    int i = 0;
    interval c[MAX_DIM_NUM];
    for(i = 0; i < MAX_DIM_NUM; i++) {
        c[i].low = start.axis[i] < end.axis[i] ? start.axis[i] : end.axis[i];
        c[i].high = start.axis[i] > end.axis[i] ? start.axis[i] : end.axis[i];
    }
    intersect = overlaps(c, o);

    return intersect;
}

int line_contain(interval o[], point start, point end) {
    // region o contain one point at least return true
    if((1 == point_contain(start, o)) && (1 == point_contain(end, o))) {
        return 1;
    }
    return 0;
}

data_type get_distance(interval c, interval o){
	data_type c_center = (c.low + c.high) / 2;
	data_type o_center = (o.low + o.high) / 2;
	data_type dis = c_center - o_center;
	return (dis > 0) ? dis : (-1 * dis);
}

int compare(interval cinterval, interval ointerval) {
	if (cinterval.low <= ointerval.low) {
		return -1;
	} else {
		return 1;
	}
}

double points_distance(struct point p1, struct point p2) {
    double distance = 0;
    int i;
    for(i = 0; i < MAX_DIM_NUM; i++) {
        distance += (p1.axis[i] - p2.axis[i]) * (p1.axis[i] - p2.axis[i]); 
    }
    return sqrt(distance);
}

struct point convert2point(double *p){
    int i;
    struct point pt;
    for(i = 0; i < MAX_DIM_NUM; i++) {
        pt.axis[i] = p[i];
    }
    return pt;
}

struct point calc_intersect_point(struct point pt, struct point start, struct point end) {
    struct point ret_pt;
    double z = pt.axis[2];
    ret_pt.axis[0] = ((z - start.axis[2]) * end.axis[0] + (end.axis[2] - z) * start.axis[0]) / \
                     (end.axis[2] - start.axis[2]);
    ret_pt.axis[1] = ((z - start.axis[2]) * end.axis[1] + (end.axis[2] - z) * start.axis[1]) / \
                         (end.axis[2] - start.axis[2]);
    ret_pt.axis[2] = z;
    return ret_pt;

}


