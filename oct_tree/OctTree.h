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
#define nodeLimit 512
#define miniDistance 1e-8
#define threshold 0.97
#define NONE -1
#define SPLIT_FLAG -2

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
	OctPoint(OctPoint *p)
	{
	        p_id = p->p_id;
	        memcpy(p_xyz,p->p_xyz,sizeof(double)*3);
	        p_tid = p->p_tid;
	        pre = p->pre;
	        next = p->next;
	}
	OctPoint(IDTYPE pid, double time, double *xyz, IDTYPE tid, IDTYPE pre,
	IDTYPE next);

    OctPoint *clone();

	bool isNear(IDTYPE pid);
	~OctPoint() {
	}
	;

public:
    void printIt();
	uint32_t getByteArraySize();
	void storeToByteArray(char **data, uint32_t &len);
	void loadFromByteArray(const char *ptr);

public:
	static int getDataLen() {
		return sizeof(OctPoint);
	}
};

class Traj {
public:
	IDTYPE t_id;
	IDTYPE t_head;
	IDTYPE t_tail; //tail point in the trajectory
public:
	Traj(void) {
	}
	;
	Traj(IDTYPE id, IDTYPE head_id, IDTYPE tail_id); //the first point of this trajectory saw in dataset
	~Traj(void) {
	}
	;
public:
    void printIt();
    int printTraj();
	uint32_t getByteArraySize();
	void storeToByteArray(char **data, uint32_t &len);
	void loadFromByteArray(const char *ptr);
};

class OctLeafNode; 
class OctIdxNode; 
class OctTNode; 
class NNQueryQueue;

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
	OctTNode() {};
	OctTNode(int nid, NodeType type, double* low, double* high, int father);
	virtual ~OctTNode() {
	}
	;
	bool calOverlap(const double *low, const double *high);
    int time_overlap(const double t_low, const double t_high);
	//change the domLow/high to overlap domLow/high with this node

	bool containPoint(OctPoint *pt);
	bool containPoint(OctPoint *pt,double *low,double *high);
	bool containPoint(OctPoint *pt, int child_id);
	bool containPoint(double* xyz, int idx);
	void octsplit(double* slow, double* shigh, char idx, double* low, double* high);

    // calc the min distance between query and node
    double getMinimumDistance(double *low, double *high);

	//virtual bool split_or_not(){cout<<"base insert!"<<endl;return false;}
	virtual void nodeInsert(OctPoint *pt) {};
    virtual void printIt(){};
	virtual void nodeSplit() {
		cout << "base split!" << endl;
	}
	;
	virtual void nodeAjust() {
		cout << "base adjust!" << endl;
	}
	;
	virtual void rangeQueryNode(double *low,double *high,vector<OctPoint*> &pt_vector){
		cout << "base rangequery!" << endl;
	};

    virtual void getOctPoints(double *low,double *high, vector<OctPoint*> &pt_vector) {

    };

	virtual void NNQueryNode(double *low,double *high,vector<OctPoint *> &pt_vector){
		cout << "base rangequery!" << endl;
	};

public:
    //void printIt();
	uint32_t getByteArraySize();
	void storeToByteArray(char **pdata, uint32_t &len);
	void loadFromByteArray(const char *ptr);

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
    
    void retrieveTrajs(double *low, double *high, double fm, hash_map<IDTYPE, Traj*> &new_trajs);

	void rangeQueryNode(double *low,double *high,vector<OctPoint*> &pt_vector);
	void NNQueryNode(double *low,double *high,vector<OctPoint *> &pt_vector);
    void getOctPoints(double *low,double *high, vector<OctPoint*> &pt_vector);
	void nodeInsert(OctPoint *pt);
    void printIt();
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
    void printIt();
	void rangeQueryNode(double *low,double *high,vector<OctPoint*> &pt_vector);
	void NNQueryNode(double *low,double *high,vector<OctPoint *> &pt_vector);
    void getOctPoints(double *low,double *high, vector<OctPoint*> &pt_vector);
    int sum(int *a);
    void specialInsert(OctPoint *pt);
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

	bool containPoint(OctPoint *pt);
	void treeSplit(bool getLow);
	void setDom(double *low, double *high, bool getLow, int &condition);
	void copy(OctTNode *root, double *treeNewLow, double *treeNewHigh);

	void rangeQuery(double *low,double *high,vector<OctPoint*> &pt_vector);
	void NNQuery(double *low,double *high,vector<OctPoint *> &pt_vector);
    void nearestNeighborQuery(double *low, double *high, hash_map<IDTYPE, Traj*> &trajs);
	void insertBetweenServer();
    void geneBorderPoint(OctPoint *pt1,OctPoint *pt2,OctPoint  *result_point, double *low, double* high);
    int pointInWhichNode(OctPoint *pt);

    // visit by class
    static int recreateTrajs(vector<OctPoint *> pt_vector, hash_map<IDTYPE, Traj*> &trajs);

};

// used for NNQueryQueue
class NNEntry{
public:
    NNEntry(OctTNode *new_node, double dist):m_pNode(new_node), m_iDist(dist){};
    ~NNEntry(){};
    OctTNode * m_pNode;
    // distance between query and node;
    double m_iDist;
};

class NNQueryQueue {
public:

    std::vector<NNEntry *> m_queue;
    NNQueryQueue(){};
    ~NNQueryQueue(){};

    void push(OctTNode *, double dist);
    OctTNode *pop();
    void deleteOctTNode(double fm);
    bool empty(){return m_queue.empty();}
}; //NNQueryQueue

typedef struct min_max_st{
    double start, end;
    double dist;
}min_max_st;


void printNodes();

void printPoints();
void printTrajs();

extern hash_map<IDTYPE, OctPoint*> g_PtList;
extern IDTYPE g_ptCount;  //without delete
extern IDTYPE g_ptNewCount;
extern hash_map<int, OctTNode*> g_NodeList;

extern int g_nodeCount;  //without delete
extern int g_leafCount;

extern hash_set<IDTYPE> g_tid;

extern hash_map<IDTYPE, Traj*> g_TrajList;

extern hash_map<int, OctTNode*> node_list;
extern hash_map<IDTYPE, OctPoint*> point_list;
extern hash_map<IDTYPE,Traj*> traj_list;


