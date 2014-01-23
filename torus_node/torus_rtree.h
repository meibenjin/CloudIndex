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
#include "spatialindex/capi/ObjVisitor.h"
#include "spatialindex/capi/CountVisitor.h"

using namespace SpatialIndex;
using namespace std;

class MyVisitor : public ObjVisitor {
public:
	//MyVisitor() : m_indexIO(0), m_leafIO(0), m_dataIO(0) {}

	void visitNode(const INode& n);

    int getNumberOfData();

    void printData();

	void visitData(std::vector<const SpatialIndex::IData*>& v);
};


// insert data into rtree
int rtree_insert(int id, double plow[], double phigh[], ISpatialIndex *rtree);

int rtree_range_query(double plow[], double phigh[], ISpatialIndex *rtree, MyVisitor& vis);

// delete data into rtree
int rtree_delete(int id, double plow[], double phigh[], ISpatialIndex *rtree);

// the the number of result by query (plow -> phigh)
int rtree_query_count(double plow[], double phigh[], ISpatialIndex *rtree);
// query data into rtree(return the number of data been fetched)
int rtree_query(double plow[], double phigh[], ISpatialIndex *rtree);

size_t rtree_get_utilization(ISpatialIndex *rtree);

//Create a new, empty, RTree
ISpatialIndex* rtree_create();

#endif /* TORUS_RTREE_H_ */
