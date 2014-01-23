/*
 * torus_rtree.c
 *
 *  Created on: Sep 24, 2013
 *      Author: meibenjin
 */


extern "C" {
#include "utils.h"
#include "logs/log.h"
};

#include "torus_rtree.h"

void MyVisitor::visitNode(const INode& n)
{
    //if (n.isLeaf()) m_leafIO++;
    //else m_indexIO++;
}

int MyVisitor::getNumberOfData() {
    return GetResultCount();
}

void MyVisitor::printData() {
    std::vector<SpatialIndex::IData*>::iterator it;
    //int len = 0;
    char buf[1024];
    for(it = GetResults().begin(); it != GetResults().end(); it++) {
        byte* pData = 0;
        uint32_t cLen = 0;
        (*it)->getData(cLen, &pData);
        //#ifdef WRITE_LOG 
            memset(buf, 0, 1024);
            char *res = reinterpret_cast<char*>(pData);
            sprintf(buf, "%s\n", res);
            //sprintf(buf + len, "leaf:%lu index:%lu data:%lu\n", m_leafIO, m_indexIO, m_dataIO);
            write_log(RESULT_LOG, buf);
        //#endif
    }
}

void MyVisitor::visitData(std::vector<const SpatialIndex::IData*>& v)
{
    //cout << v[0]->getIdentifier() << " " << v[1]->getIdentifier() << endl;
}

int rtree_insert(int id, double plow[], double phigh[], ISpatialIndex *rtree) {
    try {
        Region r = Region(plow, phigh, MAX_DIM_NUM);

        std::ostringstream os;
        os << r;
        std::string data = os.str();

        rtree->insertData(data.size() + 1, reinterpret_cast<const byte*>(data.c_str()), r, id);
    } catch (Tools::Exception& e) {
		std::cerr << "******ERROR******" << std::endl;
		std::string s = e.what();
		std::cerr << s << std::endl;
		return FALSE;
	} catch (...) {
		cerr << "******ERROR******" << endl;
		cerr << "other exception" << endl;
		return FALSE;
	}
    return TRUE;
}

int rtree_range_query(double plow[], double phigh[], ISpatialIndex *rtree, MyVisitor &vis) {
    uint32_t queryType = 0;
	try {
        if (queryType == 0) {
            // this will find all data that intersect with the query range.
            Region r = Region(plow, phigh, MAX_DIM_NUM);
            rtree->intersectsWithQuery(r, vis);
            vis.printData();
        } else if (queryType == 1) {
            // this will find the 10 nearest neighbors.
            Point p = Point(plow, MAX_DIM_NUM);
            rtree->nearestNeighborQuery(10, p, vis);
        } else if(queryType == 2) {
            Region r = Region(plow, phigh, MAX_DIM_NUM);
            rtree->selfJoinQuery(r, vis);
        } else {
            // this will find all data that is contained by the query range.
            Region r = Region(plow, phigh, MAX_DIM_NUM);
            rtree->containsWhatQuery(r, vis);
        }

    } catch (Tools::Exception& e) {
		std::cerr << "******ERROR******" << std::endl;
		std::string s = e.what();
		std::cerr << s << std::endl;
		return FALSE;
    } catch (...) { cerr << "******ERROR******" << endl;
		cerr << "other exception" << endl;
		return FALSE;
	}
    return TRUE;
}

int rtree_delete(int id, double plow[], double phigh[], ISpatialIndex *rtree) {
    try {
        Region r = Region(plow, phigh, MAX_DIM_NUM);
        if (rtree->deleteData(r, id) == false) {
            std::cerr << "******ERROR******" << std::endl;
            return FALSE;
        }

    } catch (Tools::Exception& e) {
		std::cerr << "******ERROR******" << std::endl;
		std::string s = e.what();
		std::cerr << s << std::endl;
		return FALSE;
	} catch (...) {
		cerr << "******ERROR******" << endl;
		cerr << "other exception" << endl;
		return FALSE;
	}
    return TRUE;
}

int rtree_query_count(double plow[], double phigh[], ISpatialIndex *rtree) {
    uint32_t queryType = 0;
    CountVisitor vis;

	try {

        if (queryType == 0) {
            // this will find all data that intersect with the query range.
            Region r = Region(plow, phigh, MAX_DIM_NUM);
            rtree->intersectsWithQuery(r, vis);
            //vis.printData();
        } else if (queryType == 1) {
            // this will find the 10 nearest neighbors.
            Point p = Point(plow, MAX_DIM_NUM);
            rtree->nearestNeighborQuery(10, p, vis);
        } else if(queryType == 2) {
            Region r = Region(plow, phigh, MAX_DIM_NUM);
            rtree->selfJoinQuery(r, vis);
        } else {
            // this will find all data that is contained by the query range.
            Region r = Region(plow, phigh, MAX_DIM_NUM);
            rtree->containsWhatQuery(r, vis);
        }

    } catch (Tools::Exception& e) {
		std::cerr << "******ERROR******" << std::endl;
		std::string s = e.what();
		std::cerr << s << std::endl;
		return FALSE;
    } catch (...) { cerr << "******ERROR******" << endl;
		cerr << "other exception" << endl;
		return FALSE;
	}
    return vis.GetResultCount();
}

int rtree_query(double plow[], double phigh[], ISpatialIndex *rtree) {

    uint32_t queryType = 0;
    MyVisitor vis;

	try {

        if (queryType == 0) {
            // this will find all data that intersect with the query range.
            Region r = Region(plow, phigh, MAX_DIM_NUM);
            rtree->intersectsWithQuery(r, vis);
            //vis.printData();
        } else if (queryType == 1) {
            // this will find the 10 nearest neighbors.
            Point p = Point(plow, MAX_DIM_NUM);
            rtree->nearestNeighborQuery(10, p, vis);
        } else if(queryType == 2) {
            Region r = Region(plow, phigh, MAX_DIM_NUM);
            rtree->selfJoinQuery(r, vis);
        } else {
            // this will find all data that is contained by the query range.
            Region r = Region(plow, phigh, MAX_DIM_NUM);
            rtree->containsWhatQuery(r, vis);
        }

    } catch (Tools::Exception& e) {
		std::cerr << "******ERROR******" << std::endl;
		std::string s = e.what();
		std::cerr << s << std::endl;
		return FALSE;
    } catch (...) { cerr << "******ERROR******" << endl;
		cerr << "other exception" << endl;
		return FALSE;
	}
    return vis.getNumberOfData();
}

size_t rtree_get_utilization(ISpatialIndex *rtree) {

    IStatistics *statistic; 
    rtree->getStatistics(&statistic);
    size_t nodes_num = statistic->getNumberOfData();

    return nodes_num;
}

ISpatialIndex* rtree_create()
{
    ISpatialIndex *rtree;
	try {
        // Create a new storage manager with the provided base name and a 4K page size.
        std::string baseName = "rtree";
        IStorageManager* diskfile = StorageManager::createNewDiskStorageManager(baseName, 4096);

        StorageManager::IBuffer* file = StorageManager::createNewRandomEvictionsBuffer(*diskfile, 10, false);
        // applies a main memory random buffer on top of the persistent storage manager
        // (LRU buffer, etc can be created the same way).

        // Create a new, empty, RTree with dimensionality 2, minimum load 70%, using "file" as
        // the StorageManager and the RSTAR splitting policy.
        id_type indexIdentifier;
        rtree = RTree::createNewRTree(*file, 0.7, 20, 20, MAX_DIM_NUM, SpatialIndex::RTree::RV_RSTAR, indexIdentifier);

    } catch (Tools::Exception& e) {
		std::cerr << "******ERROR******" << std::endl;
		std::string s = e.what();
		std::cerr << s << std::endl;
		return NULL;
	}
    return rtree;
}


