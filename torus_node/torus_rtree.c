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

IStorageManager* memfile;

MyDataStream::MyDataStream(std::vector<SpatialIndex::IData*> &data) : m_pNext(0) {
    m_data = data;
    m_it = m_data.begin();
    readNextEntry();
}

void MyDataStream::readNextEntry() {
    if(m_it != m_data.end()) {
        m_pNext = dynamic_cast<RTree::Data *>((*m_it)->clone());
        m_it++;
    }
}

void MyVisitor::visitNode(const INode& n)
{
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
        uint32_t len = 0;
        (*it)->getData(len, &pData);
        //#ifdef WRITE_LOG 
            memset(buf, 0, 1024);
            int t_id;
            memcpy(&t_id, pData, sizeof(int));
            sprintf(buf, "%d\n", t_id);
            write_log(RESULT_LOG, buf);
            delete[] pData;
        //#endif
    }
}

void MyVisitor::visitData(std::vector<const SpatialIndex::IData*>& v)
{
}

int rtree_insert(int t_id, int data_id, double plow[], double phigh[], ISpatialIndex *rtree) {
    if(rtree == NULL) {
        return FALSE;
    }
    try {
        Region r = Region(plow, phigh, MAX_DIM_NUM);

        //std::ostringstream os;
        //os << t_id;
        //std::string data = os.str();
        byte data[5];
        memset(data, 0, 5);
        memcpy(data, &t_id, sizeof(t_id));

        rtree->insertData(sizeof(t_id) + 1, reinterpret_cast<const byte*>(data), r, data_id);

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
    if(rtree == NULL) {
        return FALSE;
    }
    uint32_t queryType = 0;
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
    return TRUE;
}

int rtree_delete(int id, double plow[], double phigh[], ISpatialIndex *rtree) {
    if(rtree == NULL) {
        return FALSE;
    }
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
    if(rtree == NULL) {
        return -1;
    }
    uint32_t queryType = 0;
    CountVisitor vis;
	try {

        if (queryType == 0) {
            // this will find all data that intersect with the query range.
            Region r = Region(plow, phigh, MAX_DIM_NUM);
            rtree->intersectsWithQuery(r, vis);
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
    if(rtree == NULL) {
        return FALSE;
    }

    uint32_t queryType = 0;
    MyVisitor vis;
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
    return vis.getNumberOfData();
}

size_t rtree_get_utilization(ISpatialIndex *rtree) {
    if(rtree == NULL) {
        return 0;
    }

    IStatistics *statistic; 
    rtree->getStatistics(&statistic);
    size_t nodes_num = statistic->getNumberOfData();
    delete statistic;

    return nodes_num;
}


ISpatialIndex* rtree_bulkload(std::vector<SpatialIndex::IData*> &data) {
    ISpatialIndex *rtree;
	try {
        //std::string baseName = "rtree";
        //IStorageManager* diskfile = StorageManager::createNewDiskStorageManager(baseName, 4096);
        memfile = StorageManager::createNewMemoryStorageManager();

        //StorageManager::IBuffer* file = StorageManager::createNewRandomEvictionsBuffer(*memfile, 100, false);

        MyDataStream stream(data);

        //recreate new rtree
        id_type indexIdentifier;
        rtree = RTree::createAndBulkLoadNewRTree(RTree::BLM_STR, stream, *memfile, 0.7, 100, 100, MAX_DIM_NUM, SpatialIndex::RTree::RV_RSTAR, indexIdentifier);

    } catch (Tools::Exception& e){
		std::cerr << "******ERROR******" << std::endl;
		std::string s = e.what();
		std::cerr << s << std::endl;
		return NULL;
	}
    return rtree;
}

int rtree_delete(ISpatialIndex *rtree) {
    delete rtree;
    delete memfile;
    rtree = NULL;
    memfile = NULL;
    return TRUE;
}

ISpatialIndex* rtree_create() {
    ISpatialIndex *rtree;
	try {
        // Create a new storage manager with the provided base name and a 4K page size.
        //std::string baseName = "rtree";
        //IStorageManager* diskfile = StorageManager::createNewDiskStorageManager(baseName, 4096);
        memfile = StorageManager::createNewMemoryStorageManager();

        //StorageManager::IBuffer* file = StorageManager::createNewRandomEvictionsBuffer(*memfile, 100, false);
        // applies a main memory random buffer on top of the persistent storage manager
        // (LRU buffer, etc can be created the same way).

        // Create a new, empty, RTree with dimensionality 2, minimum load 70%, using "file" as
        // the StorageManager and the RSTAR splitting policy.
        id_type indexIdentifier;
        rtree = RTree::createNewRTree(*memfile, 0.7, 100, 100, MAX_DIM_NUM, SpatialIndex::RTree::RV_RSTAR, indexIdentifier);

    } catch (Tools::Exception& e) {
		std::cerr << "******ERROR******" << std::endl;
		std::string s = e.what();
		std::cerr << s << std::endl;
		return NULL;
	}

    return rtree;
}

int rtree_insert_test() {
    ISpatialIndex *rtree;
	try {
        // Create a new storage manager with the provided base name and a 4K page size.
        //std::string baseName = "rtree";
        //IStorageManager* diskfile = StorageManager::createNewDiskStorageManager(baseName, 4096);
        memfile = StorageManager::createNewMemoryStorageManager();

        //StorageManager::IBuffer* file = StorageManager::createNewRandomEvictionsBuffer(*memfile, 100, false);
        // applies a main memory random buffer on top of the persistent storage manager
        // (LRU buffer, etc can be created the same way).

        // Create a new, empty, RTree with dimensionality 2, minimum load 70%, using "file" as
        // the StorageManager and the RSTAR splitting policy.
        id_type indexIdentifier;
        rtree = RTree::createNewRTree(*memfile, 0.7, 100, 100, MAX_DIM_NUM, SpatialIndex::RTree::RV_RSTAR, indexIdentifier);

        id_type id;
		uint32_t op;
		double x1, x2, y1, y2, z1, z2;
		double plow[3], phigh[3];

        std::ifstream fin("/root/mbj/data/data");
		while (fin)
		{
			fin >> op >> id >> x1 >> y1 >> z1 >> x2 >> y2 >> z2;
			if (! fin.good()) continue; // skip newlines, etc.

			if (op == RTREE_INSERT)
			{
				plow[0] = x1; plow[1] = y1; plow[2] = z1;
				phigh[0] = x2; phigh[1] = y2; phigh[2] = z2;

				Region r = Region(plow, phigh, 3);

				std::ostringstream os;
				os << r;
				std::string data = os.str();

				rtree->insertData(data.size() + 1, reinterpret_cast<const byte*>(data.c_str()), r, id);

			}
		}

    } catch (Tools::Exception& e) {
		std::cerr << "******ERROR******" << std::endl;
		std::string s = e.what();
		std::cerr << s << std::endl;
		return FALSE;
	}
    return TRUE;
}


