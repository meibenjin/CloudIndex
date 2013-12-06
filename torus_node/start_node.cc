/*
 * start_node.c
 *
 *  Created on: Sep 18, 2013
 *      Author: meibenjin
 */

#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include"utils.h"
#include"torus_node.h"
#include"server.h"
#include"skip_list/skip_list.h"
#include"logs/log.h"

#include <cstring>

// include library header file.
#include <spatialindex/SpatialIndex.h>

using namespace SpatialIndex;

#define INSERT 1
#define DELETE 0
#define QUERY 2

//__asm__(".symver memcpy,memcpy@GLIBC_2.2.5");

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

int create_rtree()
{
    std::ifstream fin("/root/mbj/data/data");
    if (! fin)
    {
        std::cerr << "Cannot open data file /root/mbj/data/data." << std::endl;
        return -1;
    }

    // Create a new storage manager with the provided base name and a 4K page size.
    std::string baseName = "tree";
    IStorageManager* diskfile = StorageManager::createNewDiskStorageManager(baseName, 4096);

    StorageManager::IBuffer* file = StorageManager::createNewRandomEvictionsBuffer(*diskfile, 10, false);
        // applies a main memory random buffer on top of the persistent storage manager
        // (LRU buffer, etc can be created the same way).

    // Create a new, empty, RTree with dimensionality 2, minimum load 70%, using "file" as
    // the StorageManager and the RSTAR splitting policy.
    id_type indexIdentifier;
    ISpatialIndex* tree = RTree::createNewRTree(*file, 0.7, 20, 20, 2, SpatialIndex::RTree::RV_RSTAR, indexIdentifier);
}


int main(int argc, char **argv) {
	printf("start torus node.\n");
	write_log(TORUS_NODE_LOG, "start torus node.\n");

	int server_socket;
	should_run = 1;

	// new a torus node
	the_torus_node = *new_torus_node();

	// create a new request list
	req_list = new_request();

	//the_skip_list = *new_skip_list_node();

	server_socket = new_server();
	if (server_socket == FALSE) {
		exit(1);
	}
	printf("start server.\n");
	write_log(TORUS_NODE_LOG, "start server.\n");

    create_rtree();
	printf("start server.\n");
	write_log(TORUS_NODE_LOG, "create rtree.\n");

	while (should_run) {
		int conn_socket;
		conn_socket = accept_connection(server_socket);
		if (conn_socket == FALSE) {
			// TODO: handle accept connection failed
			continue;
		}

		struct message msg;
		memset(&msg, 0, sizeof(struct message));

		// receive message through the conn_socket
		if (TRUE == receive_message(conn_socket, &msg)) {
			process_message(conn_socket, msg);
		} else {
			//  TODO: handle receive message failed
			printf("receive message failed.\n");
		}
		close(conn_socket);
	}

	return 0;
}

