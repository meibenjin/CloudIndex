/*
 * geometry.h
 *
 *  Created on: Jan 15, 2015
 *      Author: meibenjin
 */

#ifndef GEOMETRY_H_
#define GEOMETRY_H_

#include "utils.h"

/* check whether two MAX_DIM_NUM dimension regions overlapped
 * note that this function remain correct when 
 * one of the two region is a point(low = high) 
 *
 * */
int overlaps(interval c[], interval o[]);

/* check whether two one dimension region overlapped
 * return: if cinterval is on the left side of ointerval return -1
 *         if cinterval is on the right side of ointerval, return 1
 *         else return 0
 * e.g.
 * case 1: cinterval: ++++
 *         ointerval:      ++++
 *         return: -1
 * case 2: cinterval:      ++++
 *         ointerval: ++++
 *         return: 1
 * case 3: cinterval:   ++++
 *         ointerval: ++++
 *         return: 0
 *
 * */
int interval_overlap(interval cinterval, interval ointerval);

// check whether the region o contains the point p
int point_contain(point p, interval o[]);

// check whether the region o intersect with the line from start to end 
int line_intersect(interval o[], point start, point end);

// check whether the region o contains the line from start to end 
int line_contain(interval o[], point start, point end);

// calc the distance of two region
data_type get_distance(interval c, interval o);

/* compare two region
 * return: if cinterval is on the left of ointerval return -1
 *         else return 1
 * e.g.
 * case 1: cinterval: ++++          ++++
 *         ointerval:      ++++       ++++
 *         return: -1
 * case 2: cinterval:      ++++         ++++
 *         ointerval: ++++            ++++
 *         return: 1
 *
 * */
int compare(interval cinterval, interval ointerval);

// calc the distance of two point
double points_distance(struct point p1, struct point p2);

#endif

