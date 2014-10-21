#include "OctTree.h"
#include<iostream>
#include<string>
#include <fstream>

vector<OctPoint*> g_PT;

int printarray[1000];
int printarray2[1000];
int count1 = 0;
int count2 = 0;

void print_tree(int nid) {
	OctTNode *current = g_NodeList.find(nid)->second;
	if (current->n_type == LEAF) {
		printarray[count1] = current->n_id;
		for (int cc = 0; cc <= count1; cc++) {

			//cout<<printarray[cc]<<"("<<g_NodeList.at(printarray[cc])->n_ptCount<<") ";
			cout << printarray[cc] << "("
					<< g_NodeList.find(printarray[cc])->second->n_ptCount
					<< ") ";
		}
		cout << endl;
	} else {
		printarray[count1++] = current->n_id;
		for (int i = 0; i < 8; i++) {
			if (current->n_children[i] != -1) {
				print_tree(current->n_children[i]);
			}
		}
		count1--;
	}

}
void print_treeNew(int nid) {
	OctTNode *current = node_list.find(nid)->second;
	if (current->n_type == LEAF) {
		printarray2[count2] = current->n_id;
		for (int cc = 0; cc <= count2; cc++) {
			cout << printarray2[cc] << "("
					<< node_list.find(printarray2[cc])->second->n_ptCount
					<< ") ";
		}
		cout << endl;
	} else {
		printarray2[count2++] = current->n_id;
		for (int i = 0; i < 8; i++) {
			if (current->n_children[i] != -1) {
				print_treeNew(current->n_children[i]);
			}
		}
		count2--;
	}

}

int loadPT(string inf_str) {
	ifstream inf;

	inf.open(inf_str.c_str());
	g_PT.clear();

	IDTYPE id;
	double xyz[3];
	IDTYPE tid;
	while (!inf.eof()) {
		int tmp;
		inf >> tmp;
		inf >> id;
		inf >> xyz[0] >> xyz[1] >> xyz[2] >> tid;

		OctPoint *pt = new OctPoint(id, -1, xyz, tid, 0, 0); //ºó¼ÌÖ¸Õë¶¼Îª¿Õ
		/*		cout<<pt1->p_xyz[0]<<","<<pt1->p_xyz[1]<<","<<pt1->p_xyz[2]<<endl;*/
// 		int i;
// 		cin>>i;
		g_PT.push_back(pt);
	}
	inf.close();
	return 0;
}

int main() {
	string file = "D://data.txt";
	string objFile = string(file);
	cout << "start loading points!" << endl;
	loadPT(objFile);
	cout << "finish loading points!" << endl;

	double high[3] = { 40.080086, 116.613917, 39996.595680 };
	double low[3] = { 39.887094, 116.262640, 39744.120000 };

	OctTree tree(low, high);

	unsigned int i = 0;
	for (i = 0; i < g_PT.size(); i++) {

		//cout<<i<<endl;
		if (tree.containPoint(g_PT.at(i))) {
			if (i % 1000 == 0)
				cout << i << endl;
			tree.treeInsert(g_PT.at(i));
		}
	}

	tree.treeSplit(true);

	tree.countNode();

	cout << "Node count:" << g_NodeList.size() << endl;
	cout << "Node count2:" << node_list.size() << endl;

//    double low1[3] = {39.984701,116.318416,39744.120184};
//    double high1[3] = {39.984803,116.318818,39744.120286};
//    vector<OctPoint*> pt_vector;
//    tree.rangeQuery(low1,high1,pt_vector);
//    cout<<"vector size:"<<pt_vector.size()<<endl;
	double tmp[3] = { 39.984702, 116.318417, 39744.120185 };
	OctPoint *ptt = new OctPoint(-1, -1, tmp, 1, 0, 0);

	cout << "in which child :" << tree.pointInWhichNode(ptt) << endl;

	hash_map<int, OctTNode*>::iterator iter;
	int countLeaf = 0;
	int countIdx = 0;
	for (iter = g_NodeList.begin(); iter != g_NodeList.end(); iter++) {
		if (iter->second->n_type == LEAF)
			countLeaf++;
		if (iter->second->n_type == IDX)
			countIdx++;

	}
	cout << countLeaf << " " << countIdx << endl;

	return 0;
}

