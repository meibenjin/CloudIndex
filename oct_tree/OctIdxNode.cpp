#include "OctTree.h"
#include "utils.h"

OctIdxNode::OctIdxNode(int nid, NodeType type, double* low, double* high,
		int father) {
	n_id = nid;
	n_type = type;
	n_ptCount = 0;
	for (int i = 0; i < 3; i++) {
		n_domLow[i] = low[i];
		n_domHigh[i] = high[i];
	}
	n_father = father;
	for (int i = 0; i < 8; i++) {
		n_children[i] = -1;
	}
}

void OctIdxNode::printIt() {
    FILE *fp;
    char buffer[1024];
    fp = fopen(RESULT_LOG, "ab+");
    if (fp == NULL) {
        printf("log: open file %s error.\n", RESULT_LOG);
        return;
    }
    int i = 0, len = 0;
    len = sprintf(buffer, "idx nid:%d ntype:%d nfater:%d ncount:%d children[", n_id, n_type, n_father, n_ptCount);
    for(i = 0; i < 8; i++) {
        len += sprintf(buffer + len, "%d ", n_children[i]);
    } 
    len += sprintf(buffer + len, "] data[");

	hash_set<IDTYPE>::iterator it;
	for (it = data.begin(); it != data.end(); it++) {
        len += sprintf(buffer + len, "%d ", *it);
	}
    len += sprintf(buffer + len, "]\n");

    fwrite(buffer, strlen(buffer), 1, fp);
    fclose(fp);
}

int OctIdxNode::sum(int *a)
{
    int i;
    int sum1=0,sum2=0;
    for(i=0; i<4; i++)
    {
        sum1 += n_children[a[i]];
        sum2 += n_children[a[i+4]];
    }
    return sum1 < sum2 ? sum1 : sum2;
}

void OctIdxNode::specialInsert(OctPoint *pt)
{
    //将为-1的child全new 一个OctTNode
    int i;
    int z_axis[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };
    int y_axis[8] = { 0, 1, 4, 5, 2, 3, 6, 7 };
    int x_axis[8] = { 0, 2, 4, 6, 1, 3, 5, 7 };
    if( -8 == sum(x_axis) )//按x轴切的
    {
        for(i=0; i<8; i++)
        {
            if( -1 == n_children[i] )//这个孩子尚没有点
            {
                double low[3];
                double high[3];
                octsplit(low, high, i, n_domLow, n_domHigh);
                low[0] = n_domLow[0];
                high[0] = n_domHigh[0];
                int nid = g_nodeCount + 1;
                OctLeafNode *tmp = new OctLeafNode(nid, LEAF, low, high, n_id);
                g_NodeList.insert(pair<int, OctTNode*>(nid, tmp));
                n_children[i] = nid;
                g_leafCount++;
                g_nodeCount++;
            }
        }
        for(i=0; i<8; i++)
        {
            if( -2 != n_children[i] )
            {
                OctTNode *temp = g_NodeList.find(n_children[i])->second;
                if (temp->containPoint(pt))
                {
                    temp->nodeInsert(pt);
                    break;
                }
            }
        }
    }
    else if( -8 == sum(y_axis) )
    {
        for(i=0; i<8; i++)
        {
            if( -1 == n_children[i] )//这个孩子尚没有点
            {
                double low[3];
                double high[3];
                octsplit(low, high, i, n_domLow, n_domHigh);
                low[1] = n_domLow[1];
                high[1] = n_domHigh[1];
                int nid = g_nodeCount + 1;
                OctLeafNode *tmp = new OctLeafNode(nid, LEAF, low, high, n_id);
                g_NodeList.insert(pair<int, OctTNode*>(nid, tmp));
                n_children[i] = nid;
                g_leafCount++;
                g_nodeCount++;
            }
        }
        for(i=0; i<8; i++)
        {
            if( -2 != n_children[i] )
            {
                OctTNode *temp = g_NodeList.find(n_children[i])->second;
                if (temp->containPoint(pt))
                {
                    temp->nodeInsert(pt);
                    break;
                }
            }
        }
    }
    else if( -8 == sum(z_axis) )
    {
        for(i=0; i<8; i++)
        {
            if( -1 == n_children[i] )//这个孩子尚没有点
            {
                double low[3];
                double high[3];
                octsplit(low, high, i, n_domLow, n_domHigh);
                low[2] = n_domLow[2];
                high[2] = n_domHigh[2];
                int nid = g_nodeCount + 1;
                OctLeafNode *tmp = new OctLeafNode(nid, LEAF, low, high, n_id);
                g_NodeList.insert(pair<int, OctTNode*>(nid, tmp));
                n_children[i] = nid;
                g_leafCount++;
                g_nodeCount++;
            }
        }
        for(i=0; i<8; i++)
        {
            if( -2 != n_children[i] )
            {
                OctTNode *temp = g_NodeList.find(n_children[i])->second;
                if (temp->containPoint(pt))
                {
                    temp->nodeInsert(pt);
                    break;
                }
            }
        }
    }
    else
    {
        cout<<"wrong!!"<<endl;
    }

}

void OctIdxNode::nodeInsert(OctPoint *pt) {
	int i;
	n_ptCount++;
    if(n_id==1)//对于根节点采用特殊处理逻辑
    {
        int split_flag = 0;
        for( i = 0; i<8; i++)
        {
            if( -2 == n_children[i] )
            {
                split_flag = 1;
            }
        }
        if( 1 == split_flag )
        {
            specialInsert(pt);
            return;
        }
    }
	for (i = 0; i < 8; i++) {
		if (!containPoint(pt, i))
			continue;
		if (-1 == n_children[i]) { //child is null
			int nid = g_nodeCount + 1;
			NodeType type = LEAF;
			double low[3];
			double high[3];
			octsplit(low, high, i, n_domLow, n_domHigh);
			int father = n_id;

			//OctTNode tmp = new OctLeafNode(nid,type,low,high,father);
			OctLeafNode *tmp = new OctLeafNode(nid, type, low, high, father);

			//g_NodeList.insert(make_pair(nid,tmp));//add to global data structure
			g_NodeList.insert(pair<int, OctTNode*>(nid, tmp));
			n_children[i] = nid;

			g_leafCount++;
			g_nodeCount++;
		}
		OctTNode *temp = g_NodeList.find(n_children[i])->second;
		if (temp->containPoint(pt)) {
			temp->nodeInsert(pt);
			//g_NodeList.insert(make_pair(temp->n_id,temp));
			break;
		}
		delete temp;
	}

}

void OctIdxNode::rangeQueryNode(double *low, double *high, vector<OctPoint*> &pt_vector) {
	for (int i = 0; i < 8; i++) {
		if (n_children[i] == -1)
			continue;

        OctTNode *tmp = g_NodeList.find(n_children[i])->second;
		//octsplit(slow, shigh, i, low, high);
		if (tmp->calOverlap(low, high)) {
            //tmp->printIt();
			//cout<<tmp->n_id<<" "<<tmp->n_type<<endl;
			tmp->rangeQueryNode(low, high, pt_vector);
		}
	}

}

void OctIdxNode::NNQueryNode(double *low,double *high,vector<OctPoint*> &pt_vector) {
	for (int i = 0; i < 8; i++) {
		if (n_children[i] == -1)
			continue;

        OctTNode *tmp = g_NodeList.find(n_children[i])->second;
		if (tmp->calOverlap(low, high)) {
			tmp->NNQueryNode(low, high, pt_vector);
		}
	}

}
