 #include "OctTree.h"

OctIdxNode::OctIdxNode(int nid,NodeType type,double* low,double* high,int father){
	n_id = nid;
	n_type = type;
	n_ptCount = 0;
	for(int i=0;i<3;i++){
		n_domLow[i] = low[i];
		n_domHigh[i] = high[i];
	}
	n_father = father;
	for(int i=0;i<8;i++){
		n_children[i] = -1;
	}
}
void OctIdxNode::nodeInsert(OctPoint *pt){
	n_ptCount++;
	int i;
	for(i=0;i<8;i++){
		if(!containPoint(pt,i))continue;
		if(-1 == n_children[i]){ //child is null
			int nid = g_nodeCount+1;
			NodeType type = LEAF;
			double low[3];
			double high[3];
			octsplit(low,high,i,n_domLow,n_domHigh);
			int father = n_id;

			//OctTNode tmp = new OctLeafNode(nid,type,low,high,father); 
			OctLeafNode *tmp = new OctLeafNode(nid,type,low,high,father); 

			//g_NodeList.insert(make_pair(nid,tmp));//add to global data structure
			g_NodeList.insert(pair<int,OctTNode*>(nid,tmp));
			n_children[i] = nid;

			g_leafCount++;
			g_nodeCount++;
		}
		OctTNode *temp = g_NodeList.find(n_children[i])->second;
		if(temp->containPoint(pt)){
			temp->nodeInsert(pt);
			//g_NodeList.insert(make_pair(temp->n_id,temp));
			break;
		}
		delete temp;
	}

}

