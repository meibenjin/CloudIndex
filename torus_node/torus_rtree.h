/*
 * torus_rtree.h
 *
 *  Created on: Sep 24, 2013
 *      Author: meibenjin
 */

#ifndef TORUS_RTREE_H_
#define TORUS_RTREE_H_


#include<cstring>
// include library header file.
#include "spatialindex/SpatialIndex.h"

using namespace SpatialIndex;
using namespace std;

int rtree_query(int op, int id, double plow[], double phigh[], int d, ISpatialIndex *rtree);

ISpatialIndex* rtree_load();

#endif /* TORUS_RTREE_H_ */
