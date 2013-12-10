/*
 * torus_rtree.c
 *
 *  Created on: Sep 24, 2013
 *      Author: meibenjin
 */

#include <cstring>
#include "torus_rtree.h"

#define INSERT 1
#define DELETE 0
#define QUERY 2


class MyVisitor : public IVisitor
{
public:
	void visitNode(const INode& n) {}

	void visitData(const IData& d)
	{
		std::cout << d.getIdentifier() << std::endl;
        // the ID of this data entry is an answer to the query. I will just print it to stdout.
	}

	void visitData(std::vector<const IData*>& v) {}
};

ISpatialIndex* load_rtree()
{
    ISpatialIndex *tree;
	try
	{
        std::ifstream fin("/root/mbj/data/data");
        if (! fin)
        {
            std::cerr << "Cannot open data file /root/mbj/data/data." << std::endl;
            return NULL;
        }

        // Create a new storage manager with the provided base name and a 4K page size.
        std::string baseName = "rtree";
        IStorageManager* diskfile = StorageManager::createNewDiskStorageManager(baseName, 4096);

        StorageManager::IBuffer* file = StorageManager::createNewRandomEvictionsBuffer(*diskfile, 10, false);
        // applies a main memory random buffer on top of the persistent storage manager
        // (LRU buffer, etc can be created the same way).

        // Create a new, empty, RTree with dimensionality 2, minimum load 70%, using "file" as
        // the StorageManager and the RSTAR splitting policy.
        id_type indexIdentifier;
        tree = RTree::createNewRTree(*file, 0.7, 20, 20, 3, SpatialIndex::RTree::RV_RSTAR, indexIdentifier);

        size_t count = 0;
        id_type id;
        uint32_t op;
        double x1, x2, y1, y2, z1, z2;
        double plow[3], phigh[3];

        uint32_t queryType = 0;

        // read data file
        while (fin)
        {
            fin >> op >> id >> x1 >> y1 >> z1 >> x2 >> y2 >> z2;
            if (! fin.good()) continue; // skip newlines, etc.

            if (op == INSERT)
            {
                plow[0] = x1; plow[1] = y1; plow[2] = z1;
                phigh[0] = x2; phigh[1] = y2; phigh[2] = z2;
                Region r = Region(plow, phigh, 3);

                std::ostringstream os;
                os << r;
                std::string data = os.str();

                tree->insertData(data.size() + 1, reinterpret_cast<const byte*>(data.c_str()), r, id);

                //tree->insertData(0, 0, r, id);
                // example of passing zero size and a null pointer as the associated data.
            }
            else if (op == DELETE)
            {
                plow[0] = x1; plow[1] = y1; plow[2] = z1;
                phigh[0] = x2; phigh[1] = y2; phigh[2] = z2;
                Region r = Region(plow, phigh, 3);

                if (tree->deleteData(r, id) == false)
                {
                    std::cerr << "******ERROR******" << std::endl;
                    std::cerr << "Cannot delete id: " << id << " , count: " << count << std::endl;
                    return NULL;
                }
            }
            else if (op == QUERY)
            {
                plow[0] = x1; plow[1] = y1; plow[2] = z1;
                phigh[0] = x2; phigh[1] = y2; phigh[2] = z2;

                MyVisitor vis;

                if (queryType == 0)
                {
                    Region r = Region(plow, phigh, 3);
                    tree->intersectsWithQuery(r, vis);
                    // this will find all data that intersect with the query range.
                }
                else if (queryType == 1)
                {
                    Point p = Point(plow, 3);
                    tree->nearestNeighborQuery(10, p, vis);
                    // this will find the 10 nearest neighbors.
                }
                else if(queryType == 2)
                {
                    Region r = Region(plow, phigh, 3);
                    tree->selfJoinQuery(r, vis);
                }
                else
                {
                    Region r = Region(plow, phigh, 3);
                    tree->containsWhatQuery(r, vis);
                    // this will find all data that is contained by the query range.
                }
            }

            /*if ((count % 1000) == 0)
                std::cerr << count << std::endl;*/
            count++;
        }
    }
	catch (Tools::Exception& e)
	{
		std::cerr << "******ERROR******" << std::endl;
		std::string s = e.what();
		std::cerr << s << std::endl;
		return NULL;
	}
    return tree;
}
