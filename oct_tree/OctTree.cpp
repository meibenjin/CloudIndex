#include "OctTree.h"

hash_map<IDTYPE, OctPoint*> g_PtList;
IDTYPE g_ptCount=0;  //without delete
IDTYPE g_ptNewCount=0;
hash_map<int,OctTNode*> g_NodeList;
hash_map<IDTYPE,Traj*> g_TrajList;

hash_map<int,OctTNode*> node_list;
hash_map<IDTYPE,OctPoint*> point_list;

int g_nodeCount=0;  //without delete
int g_leafCount=0;

hash_set<IDTYPE> g_tid;



OctTree::OctTree(double *low,double *high){
	memcpy(tree_domLow,low,sizeof(double)*3);
	memcpy(tree_domHigh,high,sizeof(double)*3);


	int nid = 1;
	NodeType type = LEAF;
	double root_low[3];
	double root_high[3];
	memcpy(root_low,low,sizeof(double)*3);
	memcpy(root_high,high,sizeof(double)*3);
	int father = -1;
	OctLeafNode *root = new OctLeafNode(nid,type,low,high,father);
	/*root(nid,type,low,high,father);*/
	//g_NodeList.insert(make_pair(nid,root));//add to global data structure
	g_NodeList.insert(pair<int,OctTNode*>(nid,root));
	g_nodeCount = 1;
	tree_root = 1;

	g_leafCount = 1;

}


void OctTree::treeInsert(OctPoint *pt){

	if(g_TrajList.find(pt->p_tid) == g_TrajList.end()){
		Traj *t = new Traj(pt->p_tid,pt->p_id,pt->p_id);
		//g_TrajList.insert(make_pair(pt->p_tid,t));
		g_TrajList.insert(pair<int,Traj*>(pt->p_tid,t));
		pt->pre = 0;
		pt->next = 0;
	}
	else{
		Traj *tmp = g_TrajList.find(pt->p_tid)->second;
		if(pt->isNear(tmp->t_tail)){
			return;
		}
		else
		{
			g_PtList.find(tmp->t_tail)->second->next = pt->p_id;
			pt->pre = tmp->t_tail;
			tmp->t_tail =pt->p_id;
		}
	}


	OctTNode *root = g_NodeList.find(tree_root)->second;
	if(root->containPoint(pt)){

		if(pt->pre!=0){
			OctPoint *p =g_PtList.find(pt->pre)->second;
			p->next = pt->p_id;
		}
		root->nodeInsert(pt);
		g_ptCount++;
		//g_NodeList.insert(make_pair(tree_root,root));
	}
	else{
//  		cout<<pt->p_id<<" "<<pt->p_xyz[0]<<","<<pt->p_xyz[1]<<","<<pt->p_xyz[2]<<endl;
//   		cout<<root->n_domLow[0]<<","<<root->n_domLow[1]<<","<<root->n_domLow[2]<<endl;
//    		cout<<root->n_domHigh[0]<<","<<root->n_domHigh[1]<<","<<root->n_domHigh[2]<<endl;
		cout<<"point not in this oct-tree!"<<endl;

	}

}
int OctTree::countNode(){
	cout<<"Node count:"<<g_nodeCount<<" Point count:"<<g_ptCount<<" New Point Count:"<<g_ptNewCount<<endl;
	return g_nodeCount;
}

bool OctTree::containPoint(OctPoint *pt)
{
	for(int i=0; i<3; i++)
		if(pt->p_xyz[i] < tree_domLow[i]-eps || pt->p_xyz[i] > tree_domHigh[i]+eps)
		{
			return false;
		}
		return true;
}

void OctTree::setDom(double *low,double *high,bool getLow,int &condition){
	//find which axis to cut
	int x_axis[8] = {0,1,2,3,4,5,6,7};
	int y_axis[8] = {0,1,4,5,2,3,6,7};
	int z_axis[8] = {0,2,4,6,1,3,5,7};
	int countX_low = 0;
	int countY_low = 0;
	int countZ_low = 0;
	int countX_high = 0;
	int countY_high = 0;
	int countZ_high = 0;
	for(int i=0;i<4;i++){
		if(-1 != g_NodeList.find(tree_root)->second->n_children[ x_axis[i] ])
			countX_low +=  g_NodeList.find( g_NodeList.find(tree_root)->second->n_children[ x_axis[i] ] )->second->n_ptCount;
		if(-1 != g_NodeList.find(tree_root)->second->n_children[ x_axis[i+4] ])
			countX_high +=  g_NodeList.find( g_NodeList.find(tree_root)->second->n_children[ x_axis[i+4] ] )->second->n_ptCount;
		if(-1 != g_NodeList.find(tree_root)->second->n_children[ y_axis[i] ])
			countY_low +=  g_NodeList.find( g_NodeList.find(tree_root)->second->n_children[ y_axis[i] ] )->second->n_ptCount;
		if(-1 != g_NodeList.find(tree_root)->second->n_children[ y_axis[i+4] ])
			countY_high +=  g_NodeList.find( g_NodeList.find(tree_root)->second->n_children[ y_axis[i+4] ] )->second->n_ptCount;
		if(-1 != g_NodeList.find(tree_root)->second->n_children[ z_axis[i] ])
			countZ_low +=  g_NodeList.find( g_NodeList.find(tree_root)->second->n_children[ z_axis[i] ] )->second->n_ptCount;
		if(-1 != g_NodeList.find(tree_root)->second->n_children[ z_axis[i+4] ])
			countZ_high +=  g_NodeList.find( g_NodeList.find(tree_root)->second->n_children[ z_axis[i+4] ] )->second->n_ptCount;
	}
	if(abs(countX_high-countX_low) < abs(countY_high-countY_low)){
		if(abs(countX_high-countX_low) < abs(countZ_high-countZ_low))
			condition = 0;
		else
			condition = 3;
	}
	else{
		if(abs(countY_high-countY_low) < abs(countZ_high-countZ_low))
			condition = 1;
		else
			condition = 3;
	}
	memcpy(low,tree_domLow,sizeof(double)*3);
	memcpy(high,tree_domHigh,sizeof(double)*3);
	switch(condition){
	case 0:
		if(getLow)
			high[0] = ( low[0] + high[0] ) /2;
		else
			low[0] = ( low[0] + high[0] ) /2;
		break;
	case 1:
		if(getLow)
			high[1] = ( low[1] + high[1] ) /2;
		else
			low[1] = ( low[1] + high[1] ) /2;
		break;
	default:
		if(getLow)
			high[2] = ( low[2] + high[2] ) /2;
		else
			low[2] = ( low[2] + high[2] ) /2;
		break;
	}
}

void OctTree::treeSplit(bool getLow){
	//find which axis to cut
	int x_axis[8] = {0,1,2,3,4,5,6,7};
	int y_axis[8] = {0,1,4,5,2,3,6,7};
	int z_axis[8] = {0,2,4,6,1,3,5,7};
	double low[3];
	double high[3];
	int condition;
	setDom(low,high,getLow,condition);//new tree dom
	OctTNode *root = new OctTNode(1,IDX,tree_domLow,tree_domHigh,-1);
	for(int i=0;i<8;i++){
		root->n_children[i] = g_NodeList.find(tree_root)->second->n_children[i];
	}
	for(int j=0;j<4;j++){
		switch(condition){
		case 0:
			if(!getLow)
				root->n_children[x_axis[j]] = -1;
			else
				root->n_children[x_axis[j+4]] = -1;
			break;
		case 1:
			if(!getLow)
				root->n_children[y_axis[j]] = -1;
			else
				root->n_children[y_axis[j+4]] = -1;
			break;
		default:
			if(!getLow)
				root->n_children[z_axis[j]] = -1;
			else
				root->n_children[z_axis[j+4]] = -1;
		}
	}

	for(int i=0;i<8;i++){
		if( -1 != root->n_children[i]){
			root->n_ptCount += g_NodeList.find(root->n_children[i])->second->n_ptCount;
		}
	}

	node_list.insert(pair<int,OctTNode*>(1,root));
	copy(root);

}

void OctTree::copy(OctTNode *n_pt){
	if(n_pt->n_type==LEAF){
		//OctTNode *tmp = g_NodeList.at(root->n_id);
		node_list.insert(pair<int,OctTNode*>(n_pt->n_id,n_pt));
		hash_set<IDTYPE>::iterator ite;
		for(ite=n_pt->data.begin();ite!=n_pt->data.end();ite++){
			OctPoint *tmp_pt = g_PtList.find(*ite)->second;
			point_list.insert(pair<IDTYPE,OctPoint*>(tmp_pt->p_id,tmp_pt));
		}
	}
	else{
		//OctTNode *tmp = g_NodeList.at(root->n_id);
		node_list.insert(pair<int,OctTNode*>(n_pt->n_id,n_pt));
		for(int i=0;i<8;i++){
			if(n_pt->n_children[i]!=-1){
				copy(g_NodeList.find(n_pt->n_children[i])->second);
			}
		}
	}


}
