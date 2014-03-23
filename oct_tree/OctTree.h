
extern "C" {
#include "utils.h"
#include "logs/log.h"
};

#include <cstdio>
#include <cmath>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cassert>
#include <cstring>
#include <set> 
#include <algorithm>
#include <ext/hash_map>
#include <ext/hash_set>
#include <map>
#include <limits>
#include <ctime>
#include <queue>
#include <iomanip>
#include <time.h>
#include<stdint.h>

using namespace std;
using namespace __gnu_cxx;
const double eps = 1e-8;

#define IDTYPE int
#define nodeLimit 2000
#define miniDistance 1e-8
#define threshold 0.97

typedef unsigned char byte;

enum NodeType {
	LEAF, IDX
};

class OctPoint {
public:
	IDTYPE p_id; //point id
	double p_time; //or double?
	short p_level; //0:in dataset,>0:newly added
	double p_xyz[3]; //coordinate xyz
	IDTYPE p_tid; //id of trajectory it belongs to
	IDTYPE pre; //pre-point in its trajectory
	IDTYPE next; //next point in its trajectory

public:
	OctPoint() {
	}
	;
	OctPoint(IDTYPE pid, double time, double *xyz, IDTYPE tid, IDTYPE pre,
	IDTYPE next);

	bool isNear(IDTYPE pid);
	~OctPoint() {
	}
	;

public:
	uint32_t getByteArraySize();
	void storeToByteArray(byte **data, uint32_t &len);
	void loadFromByteArray(const byte *ptr);

public:
	static int getDataLen() {
		return sizeof(OctPoint);
	}
};
class Traj {
public:
	IDTYPE t_id;IDTYPE t_head;IDTYPE t_tail; //tail point in the trajectory
public:
	Traj(void) {
	}
	;
	Traj(IDTYPE id, IDTYPE head_id, IDTYPE tail_id); //the first point of this trajectory saw in dataset
	~Traj(void) {
	}
	;
public:
	uint32_t getByteArraySize();
	void storeToByteArray(byte **data, uint32_t &len);
	void loadFromByteArray(const byte *ptr);
};

class OctTNode {
public:
	int n_id;
	NodeType n_type;  //leaf or index
	IDTYPE n_ptCount; //all the point in the area
	double n_domLow[3];
	double n_domHigh[3];
	int n_father; //father node id,-1 mean root
	int n_children[8]; //-1 represents no point in its child node
	hash_set<IDTYPE> data; //point data in the leaf node

public:
	OctTNode() {
	}
	;
	OctTNode(int nid, NodeType type, double* low, double* high, int father);
	virtual ~OctTNode() {
	}
	;
	void calOverlap(double domLow[], double domHigh[]);
	//change the domLow/high to overlap domLow/high with this node

	bool containPoint(OctPoint *pt);
	bool containPoint(OctPoint *pt, int child_id);
	bool containPoint(double* xyz, int idx);
	void octsplit(double* slow, double* shigh, char idx, double* low,
			double* high);

	//virtual bool split_or_not(){cout<<"base insert!"<<endl;return false;}
	virtual void nodeInsert(OctPoint *pt) {
		cout << "base insert!" << endl;
	}
	;
	virtual void nodeSplit() {
		cout << "base split!" << endl;
	}
	;
	virtual void nodeAjust() {
		cout << "base adjust!" << endl;
	}
	;

public:
	uint32_t getByteArraySize();
	void storeToByteArray(byte **pdata, uint32_t &len);
	void loadFromByteArray(const byte *ptr);

};
class OctIdxNode: public OctTNode {
public:
	OctIdxNode() {
	}
	;
	~OctIdxNode() {
	}
	;
	OctIdxNode(int nid, NodeType type, double* low, double* high, int father);
	void nodeInsert(OctPoint *pt);
};

class OctLeafNode: public OctTNode {
public:
	OctLeafNode(int nid, NodeType type, double* low, double* high, int father);
	OctLeafNode() {
	}
	;
	~OctLeafNode() {
	}
	;
	void nodeInsert(OctPoint *pt);
	void nodeSplit();
	void geneChildRelativeLocation(OctPoint *pt, int *l);
	void geneChildRelativeLocation(double* xyz, int* l);

	int geneChildRelativeLocation(IDTYPE pid);
	int geneChildRelativeLocation(double* xyz);
	void splitPointIntoChild(OctPoint *pt, int child_id);
	void nodeAjust();
	bool split_or_not(); //chain or not
	int judgeBetween(IDTYPE pid1, IDTYPE pid2); // judge whether through a new child?
	int judgeBetween(double* pid1, double* pid2);
	void geneMidpoint(IDTYPE pid1, IDTYPE pid2, int condition);
	void calMid(double* low, double* high, double* xyz);
	void convertToIndex();IDTYPE whichIsCloser(IDTYPE pid1, IDTYPE pid2,
	IDTYPE pid);

};
class OctTree {
public:
	int tree_root; //root node id
	double tree_domLow[3];
	double tree_domHigh[3];

	//IDTYPE tree_ptCount;
	//hash_map<int,OctIdxNode*> tree_idexSet;
	//hash_map<int,OctLeafNode*> tree_leafSet;

public:
	OctTree(double *low, double *high);
	OctTree() {
	}
	;
	~OctTree() {
	}
	;
	void treeInsert(OctPoint *pt); //return p_id
	int countNode();
	vector<IDTYPE> rangeQuery(double queryLow[], double queryHigh[]); //return the ids of points in the area

	bool containPoint(OctPoint *pt);
	void treeSplit(bool getLow);
	void setDom(double *low, double *high, bool getLow, int &condition);
	void copy(OctTNode *root);
};

extern hash_map<IDTYPE, OctPoint*> g_PtList;
extern IDTYPE g_ptCount;  //without delete
extern IDTYPE g_ptNewCount;
extern hash_map<int, OctTNode*> g_NodeList;

extern int g_nodeCount;  //without delete
extern int g_leafCount;

extern hash_set<IDTYPE> g_tid;

extern hash_map<IDTYPE, Traj*> g_TrajList;

// only used for oct tree split
extern hash_map<int, OctTNode*> node_list;
extern hash_map<IDTYPE, OctPoint*> point_list;
