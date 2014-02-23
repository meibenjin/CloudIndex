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

using namespace SpatialIndex::RTree;
using namespace SpatialIndex;
using namespace std;

class MyDataStream : public IDataStream
{
public:
	MyDataStream(std::vector<SpatialIndex::IData*> &data);
    virtual ~MyDataStream()
    {
        if (m_pNext != 0) delete m_pNext;

        std::vector<SpatialIndex::IData*>().swap(m_data);
    }

    virtual IData* getNext()
    {
        if (m_pNext == 0) return 0;

        Data *ret = m_pNext;
        m_pNext = 0;
        readNextEntry();
        return ret;
    }

    virtual bool hasNext()
    {
        return (m_pNext != 0);
    }

    virtual uint32_t size()
    {
        throw Tools::NotSupportedException("Operation not supported.");
    }

	virtual void rewind()
	{
		if (m_pNext != 0)
		{
			delete m_pNext;
			m_pNext = 0;
		}

        m_it = m_data.begin();
		readNextEntry();
	}

	void readNextEntry();

    std::vector<SpatialIndex::IData*> m_data;
    std::vector<SpatialIndex::IData*>::iterator m_it;
	Data *m_pNext;
};

class MyVisitor : public ObjVisitor {
public:

	void visitNode(const INode& n);

    int getNumberOfData();

    void printData();

	void visitData(std::vector<const SpatialIndex::IData*>& v);
};

//StorageManager::IBuffer* storage_manager_buffer;

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

ISpatialIndex* rtree_bulkload(std::vector<SpatialIndex::IData*> &data);

//Create a new, empty, RTree
ISpatialIndex* rtree_create();

int rtree_delete(ISpatialIndex *rtree);

int rtree_insert_test();

#endif /* TORUS_RTREE_H_ */
