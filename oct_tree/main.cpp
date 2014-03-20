#include "OctTree.h"
#include<iostream>
#include<string>
#include <fstream>
#include<windows.h>

vector<OctPoint*> g_PT;


int printarray[1000];
int printarray2[1000];
int count1=0;
int count2=0;

class Timer
{
private:
	LARGE_INTEGER m_t1, m_t2, m_tc;
	bool m_fstart, m_fstop;
public:
	Timer() 
	{
		m_fstart = 0;
		m_fstop = 0;
		QueryPerformanceFrequency(&m_tc);
	}
	void start()
	{
		m_fstart = 1;
		QueryPerformanceCounter(&m_t1);
	}
	void stop()
	{
		m_fstop = 1;
		QueryPerformanceCounter(&m_t2);
	}
	double interval()
	{
		if(!m_fstart || !m_fstop)
		{
			cerr << "not enough info to measure" << endl;
			return 0;
		}
		return double(m_t2.QuadPart - m_t1.QuadPart) / double(m_tc.QuadPart);
	}
};


void print_tree(int nid){
	OctTNode *current =g_NodeList.find(nid)->second;
	if(current->n_type==LEAF){
		printarray[count1]=current->n_id;
		for(int cc=0;cc<=count1;cc++){
			
			//cout<<printarray[cc]<<"("<<g_NodeList.at(printarray[cc])->n_ptCount<<") ";
			cout<<printarray[cc]<<"("<<g_NodeList.find(printarray[cc])->second->n_ptCount<<") ";
		}
		cout<<endl;
	}
	else{
		printarray[count1++]=current->n_id;
		for(int i = 0;i<8;i++){
			if(current->n_children[i]!=-1){
				print_tree(current->n_children[i]);
			}
		}
		count1--;
	}

}
void print_treeNew(int nid){
	OctTNode *current =node_list.find(nid)->second;
	if(current->n_type==LEAF){
		printarray2[count2]=current->n_id;
		for(int cc=0;cc<=count2;cc++){
			cout<<printarray2[cc]<<"("<<node_list.find(printarray2[cc])->second->n_ptCount<<") ";
		}
		cout<<endl;
	}
	else{
		printarray2[count2++]=current->n_id;
		for(int i = 0;i<8;i++){
			if(current->n_children[i]!=-1){
				print_treeNew(current->n_children[i]);
			}
		}
		count2--;
	}

}



int loadPT(string inf_str)
{
	ifstream inf;

	inf.open(inf_str.c_str());
	g_PT.clear();

	IDTYPE id;
	double xyz[3];
	double time;
	IDTYPE pre;
	IDTYPE next;
	IDTYPE tid;
	while (!inf.eof())
	{
		inf>>id;
		inf>>xyz[0]>>xyz[1]>>xyz[2]>>time>>pre>>next>>tid;

		OctPoint *pt = new OctPoint(id,time,xyz,tid,-1,-1);//后继指针都为空
/*		cout<<pt1->p_xyz[0]<<","<<pt1->p_xyz[1]<<","<<pt1->p_xyz[2]<<endl;*/
// 		int i;
// 		cin>>i;
		g_PT.push_back(pt);
	}
	inf.close();
	return 0;
}


int main()
{
	Timer tm = Timer();
	string file = "D://abc1.txt";
	string objFile = string(file); 
	cout<<"start loading points!"<<endl;
	loadPT(objFile);
	cout<<"finish loading points!"<<endl;


 	double high[3]={40.080076,116.613907,7584};
 	double low[3]={39.887104,116.26265,-466};
// 	double high[3]={9999.0,9999.0,9999.0};
// 	double low[3]={0,0,0};

	OctTree tree(low,high);

	tm.start(); 
	for(int i=0; i< g_PT.size();i++){
		
		//cout<<i<<endl;
		if(tree.containPoint(g_PT.at(i))){
			if( i%1000==0)cout<<i<<endl;
			tree.treeInsert(g_PT.at(i));
		}
	}
	tm.stop();

	//OctTNode *root = g_NodeList.find(1)->second;
	tree.treeSplit(true);

	tree.countNode();

	cout<<"the time for inserting "<<g_PT.size()<<" objects is "<< tm.interval()<<" seconds"<<endl;

	cout<<"Node count:"<<g_NodeList.size()<<endl;
	cout<<"Node count2:"<<node_list.size()<<endl;


	hash_map<int,OctTNode*>::iterator iter;
	int countLeaf =0;
	int countIdx=0;
	for(iter = g_NodeList.begin() ; iter!=g_NodeList.end() ;iter++ ){
		if(iter->second->n_type == LEAF)countLeaf++;
		if(iter->second->n_type == IDX)countIdx++;

	}
	cout<<countLeaf<<" "<<countIdx<<endl;

	freopen("D:\\result.txt","w",stdout);

	print_tree(1);
	cout<<"------------"<<endl;
	print_treeNew(1);

	cout<<"P:"<<sizeof(OctPoint)<<" TN:"<<sizeof(OctTNode)<<" Leaf:"<<sizeof(OctLeafNode)<<" Idx:"<<sizeof(OctIdxNode)<<endl;



// 
// 	int i;
// 	cin>>i;
	getchar();
	return 0; 
}




