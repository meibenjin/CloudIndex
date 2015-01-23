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

			if (op == DATA_INSERT)
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




//backup for rtree operation from server.c
/////////////////////////////

int get_split_dimension() {
    int d = 0, i, j;
    double plow[MAX_DIM_NUM], phigh[MAX_DIM_NUM];
    size_t balance_gain = -1, min_gain = -1; 
    size_t pnum = 0, nnum = 0;

    size_t c = rtree_get_utilization(the_torus_rtree);
    char buf[1024];
    memset(buf, 0, 1024);
    int len = sprintf(buf, "%lu ", c);
    for(i = 0; i < MAX_DIM_NUM; i++) {
        for (j = 0; j < MAX_DIM_NUM; j++) {
            plow[j] = (double)the_torus->info.region[j].low;
            phigh[j] = (double)the_torus->info.region[j].high;
        }
        plow[i] = (plow[i] + phigh[i]) * 0.5;
        pnum = rtree_query_count(plow, phigh, the_torus_rtree);
        nnum = c - pnum; 
        balance_gain = (pnum > nnum) ? (pnum - nnum) : (nnum - pnum);
        if(balance_gain < min_gain) {
            min_gain = balance_gain;
            d = i;
        }
        len += sprintf(buf + len, "%f ", plow[i]);
        len += sprintf(buf + len, "%lu %lu ", pnum, nnum);
    }
    len += sprintf(buf + len, "%d\n", d);
    write_log(OCT_TREE_LOG, buf);
    return d;
}

int send_splitted_rtree(char *dst_ip, double plow[], double phigh[]) {

    struct timespec start, end, s, e;
    double elasped = 0L, el;
    clock_gettime(CLOCK_REALTIME, &start);
    MyVisitor vis;
    rtree_range_query(plow, phigh, the_torus_rtree, vis);

    std::vector<SpatialIndex::IData*> v = vis.GetResults();
    std::vector<SpatialIndex::IData*>::iterator it;

    clock_gettime(CLOCK_REALTIME, &end);
    elasped = get_elasped_time(start, end);
    write_log(RESULT_LOG, "get split data spend %f ms\n", elasped / 1000000.0);
    elasped = 0L;

	int socketfd;

    // create a data socket to send rtree data
	socketfd = new_client_socket(dst_ip, COMPUTE_WORKER_PORT);
	if (FALSE == socketfd) {
		return FALSE;
	}

	// get local ip address
	char local_ip[IP_ADDR_LENGTH];
	memset(local_ip, 0, IP_ADDR_LENGTH);
    get_node_ip(the_torus->info, local_ip);
	/*if (FALSE == get_local_ip(local_ip)) {
		return FALSE;
	}*/

    struct message msg;
    msg.msg_size = calc_msg_header_size() + 1;
    fill_message(msg.msg_size, RELOAD_RTREE, local_ip, dst_ip, "", "", 1, &msg);
    send_data(socketfd, (void *)&msg, msg.msg_size);

    size_t cpy_len = sizeof(uint32_t);
    int total_len = 0; 
    uint32_t count = 0;
    RTree::Data *data;

    char buf[SOCKET_BUF_SIZE];
    memset(buf, 0, SOCKET_BUF_SIZE);

    for(it = v.begin(); it != v.end(); it++) {
        clock_gettime(CLOCK_REALTIME, &s);

        // data include data_id region and data content
        data = dynamic_cast<RTree::Data *>(*it);
        byte *package_ptr;
        uint32_t package_len;
        data->storeToByteArray(&package_ptr, package_len);

        // data num
        count++;
        memcpy(buf + 0, &count, sizeof(uint32_t));

        // package_len may be unused in do_rtree_load_data() need more test
        //memcpy(buf + cpy_len, &package_len, sizeof(uint32_t));
        //cpy_len += sizeof(uint32_t);

        memcpy(buf + cpy_len, package_ptr, package_len);
        cpy_len += package_len;

        delete[] package_ptr;

        clock_gettime(CLOCK_REALTIME, &e);
        el += get_elasped_time(s, e);

        uint32_t next_len = sizeof(uint32_t) * 2 + package_len;
        if(cpy_len + next_len < SOCKET_BUF_SIZE) {
            continue;
        } else {
            clock_gettime(CLOCK_REALTIME, &start);
            send_data(socketfd, (void *)buf, SOCKET_BUF_SIZE);
            clock_gettime(CLOCK_REALTIME, &end);
            elasped += get_elasped_time(start, end);

            total_len += cpy_len;
            count = 0;
            cpy_len = sizeof(uint32_t);
            memset(buf, 0, SOCKET_BUF_SIZE);
        }
    }
    if(cpy_len > sizeof(uint32_t)) {
        send_data(socketfd, (void *)buf, SOCKET_BUF_SIZE);
    }
	close(socketfd);

    total_len += cpy_len;
    write_log(RESULT_LOG, "total send %f k\n", total_len/ 1000.0);

    write_log(RESULT_LOG, "package split data spend %f ms\n", el/ 1000000.0);

    write_log(RESULT_LOG, "send split data spend %f ms\n", elasped/ 1000000.0);
    return TRUE;
}


int rtree_recreate(double plow[], double phigh[]){

    MyVisitor vis;
    rtree_range_query(plow, phigh, the_torus_rtree, vis);
    std::vector<SpatialIndex::IData*> v = vis.GetResults();

    // delete origional rtree 
    rtree_delete(the_torus_rtree);

    the_torus_rtree = rtree_bulkload(vis.GetResults());
    return TRUE;
}

int local_rtree_query(double low[], double high[]) {
    MyVisitor vis;
    rtree_range_query(low, high, the_torus_rtree, vis);
    std::vector<SpatialIndex::IData*> v = vis.GetResults();

    //find a idle torus node to do refinement
    char idle_ip[IP_ADDR_LENGTH];
    memset(idle_ip, 0, IP_ADDR_LENGTH);
    //find_idle_torus_node(idle_ip);

    return TRUE;
}

int operate_rtree(struct query_struct query) {

    if(the_torus_rtree == NULL) {
        write_log(ERROR_LOG, "torus rtree didn't create.\n");
        return FALSE;
    }

    int i;
    double plow[MAX_DIM_NUM], phigh[MAX_DIM_NUM];

    for (i = 0; i < MAX_DIM_NUM; i++) {
        plow[i] = (double)query.intval[i].low;
        phigh[i] = (double)query.intval[i].high;
    }

    // rtree operation 
    if(query.op == DATA_INSERT) {
        // need lock when insert rtree
        pthread_mutex_lock(&mutex);
        if(FALSE == rtree_insert(query.trajectory_id, query.data_id, plow, phigh, the_torus_rtree)) {
            return FALSE;
        }
        size_t c = rtree_get_utilization(the_torus_rtree);
        if(c >= the_torus->info.capacity){
            torus_split();
        }
        pthread_mutex_unlock(&mutex);
    } else if(query.op == DATA_DELETE) {
        if(FALSE == rtree_delete(query.data_id, plow, phigh, the_torus_rtree)) {
            return FALSE;
        }
    } else if(query.op == RANGE_QUERY) {
        // TODO add read lock
        if(FALSE == local_rtree_query(plow, phigh)) {
            return FALSE;
        }
    }
    return TRUE;
}

int do_rtree_load_data(connection_t conn, struct message msg){
    struct timespec start, end;
    double elasped = 0L;

    clock_gettime(CLOCK_REALTIME, &start);

    byte *data_ptr, *ptr;
    id_type data_id;
    uint32_t package_num, data_len;// package_len;
    Region region;
    static int count = 0;

    int length = 0, loop = 1;
    std::vector<SpatialIndex::IData *> v;

    byte buf[SOCKET_BUF_SIZE];
    memset(buf, 0, SOCKET_BUF_SIZE);

    while(loop) {
        length = recv_data(conn->socketfd, buf, SOCKET_BUF_SIZE);
        if(length == 0) {
            // client nomally closed
            close_connection(conn);
            loop = 0;
        } else {
            length = 0;
            ptr = reinterpret_cast<byte *>(buf); 
            memcpy(&package_num, ptr, sizeof(uint32_t));
            ptr += sizeof(uint32_t);
            count += package_num;
            while(package_num--){

                //memcpy(&package_len, ptr, sizeof(uint32_t));
                //ptr += sizeof(uint32_t);

                memcpy(&data_id, ptr, sizeof(id_type));
                ptr += sizeof(id_type);

                memcpy(&data_len, ptr , sizeof(uint32_t));
                ptr += sizeof(uint32_t);

                if (data_len > 0){
                    data_ptr = new byte[data_len];
                    memcpy(data_ptr, ptr, data_len);
                    ptr += data_len;
                }

                region.loadFromByteArray(ptr);
                ptr += region.getByteArraySize();

                SpatialIndex::IData *data = dynamic_cast<SpatialIndex::IData*>(new Data(data_len, data_ptr, region, data_id));
                v.push_back(data);
            }
            memset(buf, 0, SOCKET_BUF_SIZE);
        } 
    }

    clock_gettime(CLOCK_REALTIME, &end);
    elasped = get_elasped_time(start, end);

    write_log(RESULT_LOG, "receive split data %f ms\n", (double) elasped/ 1000000.0);

    //bulkload rtree
    the_torus_rtree = rtree_bulkload(v);

    std::vector<SpatialIndex::IData*>::iterator it;
    for (it = v.begin(); it != v.end(); it++) {
        delete *it;
    } 

    size_t c = rtree_get_utilization(the_torus_rtree);
    write_log(RESULT_LOG, "%lu\n", c);

    time_t now;
    now = time(NULL);
    write_log(RESULT_LOG, "%ld %d\n", now, count);
    return TRUE;
}

// send part of current torus node's rtree 
int send_splitted_rtree(char *dst_ip, double plow[], double phigh[]);

// recreate local rtree by specified region
int rtree_recreate(double plow[], double phigh[]);


// query type can be insert, delete or query
int operate_rtree(struct query_struct query);

