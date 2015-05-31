#include "OctTree.h"

extern "C" {
#include"logs/log.h"
#include "utils.h"
}

hash_map<IDTYPE, OctPoint*> g_PtList;
IDTYPE g_ptCount = 0;  //without delete
IDTYPE g_ptNewCount = 0;
hash_map<int, OctTNode*> g_NodeList;
hash_map<IDTYPE, Traj*> g_TrajList;

hash_map<int, OctTNode*> node_list;
hash_map<IDTYPE, OctPoint*> point_list;
hash_map<IDTYPE, Traj*> traj_list;

int g_nodeCount = 0;  //without delete
int g_leafCount = 0;

hash_set<IDTYPE> g_tid;

void printNodes() {
    hash_map<int, OctTNode *>::iterator it;
    for(it = g_NodeList.begin(); it != g_NodeList.end(); it++) {
        OctTNode *node = it->second;
        node->printIt();
    }
}

void printPoints() {
    hash_map<int, OctPoint *>::iterator it;
    for(it = g_PtList.begin(); it != g_PtList.end(); it++) {
        OctPoint *point= it->second;
        point->printIt();
    }
}

void printTrajs() {
    hash_map<int, Traj *>::iterator it;
    for(it = g_TrajList.begin(); it != g_TrajList.end(); it++) {
        Traj *traj= it->second;
        traj->printIt();
    }
}

OctTree::OctTree(double *low, double *high) {
	memcpy(tree_domLow, low, sizeof(double) * 3);
	memcpy(tree_domHigh, high, sizeof(double) * 3);

	int nid = 1;
	NodeType type = LEAF;
	double root_low[3];
	double root_high[3];
	memcpy(root_low, low, sizeof(double) * 3);
	memcpy(root_high, high, sizeof(double) * 3);
	int father = -1;
	OctLeafNode *root = new OctLeafNode(nid, type, low, high, father);
	/*root(nid,type,low,high,father);*/
	//g_NodeList.insert(make_pair(nid,root));//add to global data structure
	g_NodeList.insert(pair<int, OctTNode*>(nid, root));
	g_nodeCount = 1;
	tree_root = 1;

	g_leafCount = 1;

}

void OctTree::treeInsert(OctPoint *pt) {
	OctTNode *root = g_NodeList.find(tree_root)->second;
	if (root->containPoint(pt)) {
		//1.以这个点做rangequery
		//2.到于查到的点，不在本节点，且tid与这个点相同，需要做面上的插值
		//3.更新两个server的前后继值
		//4.更新两个server上的trajectory状态
		//这个点所在Trajectory不存在
		if (g_TrajList.find(pt->p_tid) == g_TrajList.end()) {

			vector<OctPoint*> pt_vector;
			double range[3] = { 1.0, 1.0, 1.0 };  //需要定义一下，以这个点为中心的一个范围
			double low[3], high[3];
			for (int i = 0; i < 3; i++) {
				low[i] = pt->p_xyz[i] - range[i];
				high[i] = pt->p_xyz[i] + range[i];
			}
			//benjin rangequery
			rangeQuery(low, high, pt_vector);  //得到包含与其相邻点的vector  ToDo Benjin
			for (size_t i = 0; i < pt_vector.size(); i++) {
				if (pt_vector[i]->p_tid == pt->p_tid &&   //相同Traj
						pt_vector[i]->p_xyz[2] < pt->p_xyz[2] && //在其前
						pt_vector[i]->next == NONE                         //next为空
								) {
					OctPoint *point_new = new OctPoint();
					g_ptNewCount++;
					//geneBorderPoint(pt_vector[i], pt, point_new);      //求出与面的交点

					point_new->p_id = -g_ptNewCount;
					point_new->p_tid = pt->p_tid;                  //补全信息
					pt->pre = point_new->p_id;
					point_new->next = pt->p_id;
					point_new->pre = NONE;                          //更新相关的next和pre

					g_PtList.insert(
							pair<IDTYPE, OctPoint*>(point_new->p_id,
									point_new));

					Traj *t = new Traj(pt->p_tid, pt->p_id, pt->p_id); // 新建一个trajectory
					g_TrajList.insert(pair<int, Traj*>(pt->p_tid, t));

					// TODO (liudehai#1#): 本金将这个点发到邻居server，
					//邻居server data域里 并 更新所在Traj的tail

				}
			}
		}

		else {
			// 找到所在Trajectory的尾部，并更新相关信息
			Traj *tmp = g_TrajList.find(pt->p_tid)->second;
			/*
			 *near函数用来判断是否是同一个点，这一步用来压缩输入数据用
			 if(pt->isNear(tmp->t_tail)){
			 return;
			 }
			 */
			g_PtList.find(tmp->t_tail)->second->next = pt->p_id;
			pt->pre = tmp->t_tail;
			tmp->t_tail = pt->p_id;
		}
	} else {
		cout << "point not in this oct-tree!" << endl;
	}
}
int OctTree::countNode() {
	cout << "Node count:" << g_nodeCount << " Point count:" << g_ptCount
			<< " New Point Count:" << g_ptNewCount << endl;
	return g_nodeCount;
}

bool OctTree::containPoint(OctPoint *pt) {
	for (int i = 0; i < 3; i++)
		if (pt->p_xyz[i] < tree_domLow[i] - eps
				|| pt->p_xyz[i] > tree_domHigh[i] + eps) {
			return false;
		}
	return true;
}

bool containPointNew(OctPoint *pt,double *low,double *high)
{
       	for (int i = 0; i < 3; i++)
		if (pt->p_xyz[i] < low[i] - eps
				|| pt->p_xyz[i] > high[i] + eps) {
			return false;
		}
	return true;
}

void OctTree::setDom(double *low, double *high, bool getLow, int &condition) {
	//find which axis to cut
	int z_axis[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };
	int y_axis[8] = { 0, 1, 4, 5, 2, 3, 6, 7 };
	int x_axis[8] = { 0, 2, 4, 6, 1, 3, 5, 7 };
	int countX_low = 0;
	int countY_low = 0;
	int countZ_low = 0;
	int countX_high = 0;
	int countY_high = 0;
	int countZ_high = 0;
	for (int i = 0; i < 4; i++) {
		if (-1 != g_NodeList.find(tree_root)->second->n_children[x_axis[i]])
			countX_low +=
					g_NodeList.find(
							g_NodeList.find(tree_root)->second->n_children[x_axis[i]])->second->n_ptCount;
		if (-1 != g_NodeList.find(tree_root)->second->n_children[x_axis[i + 4]])
			countX_high +=
					g_NodeList.find(
							g_NodeList.find(tree_root)->second->n_children[x_axis[i
									+ 4]])->second->n_ptCount;
		if (-1 != g_NodeList.find(tree_root)->second->n_children[y_axis[i]])
			countY_low +=
					g_NodeList.find(
							g_NodeList.find(tree_root)->second->n_children[y_axis[i]])->second->n_ptCount;
		if (-1 != g_NodeList.find(tree_root)->second->n_children[y_axis[i + 4]])
			countY_high +=
					g_NodeList.find(
							g_NodeList.find(tree_root)->second->n_children[y_axis[i
									+ 4]])->second->n_ptCount;
		if (-1 != g_NodeList.find(tree_root)->second->n_children[z_axis[i]])
			countZ_low +=
					g_NodeList.find(
							g_NodeList.find(tree_root)->second->n_children[z_axis[i]])->second->n_ptCount;
		if (-1 != g_NodeList.find(tree_root)->second->n_children[z_axis[i + 4]])
			countZ_high +=
					g_NodeList.find(
							g_NodeList.find(tree_root)->second->n_children[z_axis[i
									+ 4]])->second->n_ptCount;
	}
	if (abs(countX_high - countX_low) < abs(countY_high - countY_low)) {
		if (abs(countX_high - countX_low) < abs(countZ_high - countZ_low))
			condition = 0;
		else
			condition = 2;
	} else {
		if (abs(countY_high - countY_low) < abs(countZ_high - countZ_low))
			condition = 1;
		else
			condition = 2;
	}
	memcpy(low, tree_domLow, sizeof(double) * 3);
	memcpy(high, tree_domHigh, sizeof(double) * 3);
	switch (condition) {
	case 0:
		if (getLow)
			high[0] = (low[0] + high[0]) / 2;
		else
			low[0] = (low[0] + high[0]) / 2;
		break;
	case 1:
		if (getLow)
			high[1] = (low[1] + high[1]) / 2;
		else
			low[1] = (low[1] + high[1]) / 2;
		break;
	default:
		if (getLow)
			high[2] = (low[2] + high[2]) / 2;
		else
			low[2] = (low[2] + high[2]) / 2;
		break;
	}
}

void OctTree::treeSplit(bool getLow) {
    /*FILE *fp;
    char buffer[1024];
    fp = fopen(RESULT_LOG, "ab+");
    if (fp == NULL) {
        printf("log: open file %s error.\n", RESULT_LOG);
        return;
    }
    strcpy(buffer, "leaf node split here 1\n");
    fwrite(buffer, strlen(buffer), 1, fp);
    fclose(fp);*/
	//find which axis to cut
	int z_axis[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };
	int y_axis[8] = { 0, 1, 4, 5, 2, 3, 6, 7 };
	int x_axis[8] = { 0, 2, 4, 6, 1, 3, 5, 7 };
	double low[3];
	double high[3];
	int condition;
	setDom(low, high, getLow, condition);                         //new tree dom
	OctTNode *root = new OctIdxNode(1, IDX, tree_domLow, tree_domHigh, -1);
	OctTNode *now = g_NodeList.find(tree_root)->second;
	for (int i = 0; i < 8; i++) {
		root->n_children[i] = now->n_children[i];
	}

	for (int j = 0; j < 4; j++) {
		switch (condition) {
		case 0:
			if (!getLow){
				root->n_children[x_axis[j]] = SPLIT_FLAG;
			} else {
				root->n_children[x_axis[j + 4]] = SPLIT_FLAG;
			}
			break;
		case 1:
			if (!getLow){
				root->n_children[y_axis[j]] = SPLIT_FLAG;
			} else {
				root->n_children[y_axis[j + 4]] = SPLIT_FLAG;
			}
			break;
		default:
			if (!getLow) {
				root->n_children[z_axis[j]] = SPLIT_FLAG;
			} else {
				root->n_children[z_axis[j + 4]] = SPLIT_FLAG;
			}
			break;
		}
	}

	for (int i = 0; i < 8; i++) {
		if (0 <= root->n_children[i]) {
			root->n_ptCount +=
					g_NodeList.find(root->n_children[i])->second->n_ptCount;
		}
	}

	node_list.insert(pair<int, OctTNode*>(1, root));

	copy(root, low, high);
}

bool comparePointTime(IDTYPE p1, IDTYPE p2) {
	OctPoint *point1, *point2;
	if (g_PtList.find(p1) != g_PtList.end()) {
		point1 = g_PtList.find(p1)->second;
	} else {
		point1 = point_list.find(p1)->second;
	}
	if (g_PtList.find(p2) != g_PtList.end()) {
        point2 = g_PtList.find(p2)->second;
    } else {
        point2 = point_list.find(p2)->second;
    }

	if (point1->p_xyz[2] < point2->p_xyz[2]) {
		return true;
	} else
		return false;
}

void OctTree::copy(OctTNode *n_pt,double *treeNewLow,double *treeNewHigh) {
	if (n_pt->n_type == LEAF) {//叶子节点的复制
        //新的server上树的范围
		//OctTNode *tmp = g_NodeList.at(root->n_id);
		OctTNode *newnode = new OctLeafNode(n_pt->n_id,n_pt->n_type,n_pt->n_domLow,n_pt->n_domHigh,n_pt->n_father);
		newnode->n_ptCount = n_pt->n_ptCount;

		node_list.insert(pair<int, OctTNode*>(newnode->n_id, newnode)); //增加到新的server的hash_map上
		//g_NodeList.erase(n_pt->n_id);                   //本机server上NodeList相应的删除

        //printNodes();
		while (!n_pt->data.empty()) {//针对这个叶子节点中的data域，按每个Traj的顺序
			hash_set<IDTYPE>::iterator top = n_pt->data.begin();
			OctPoint *tmp = g_PtList.find(*top)->second->clone();

            //下面的if...else主要为了更新在新的server上这个点所在的Traj信息
            //如果traj_list没有这个点所在的traj,在新的server上新建一个头,尾为当前Piont的Traj
			if (traj_list.find(tmp->p_tid) == traj_list.end()) {
				Traj *trajectory = new Traj(tmp->p_tid, tmp->p_id, tmp->p_id);
				traj_list.insert(pair<IDTYPE, Traj*>(tmp->p_tid, trajectory));
			}
			else { //出现过
				Traj *tt = traj_list.find(tmp->p_tid)->second;
				if (!comparePointTime(tt->t_head, tmp->p_id)) //tmp点在trajectory  head之前
                                {
					tt->t_head = tmp->p_id;
				}
				if (comparePointTime(tt->t_tail, tmp->p_id))   //tmp 在tail 之后
						{
					tt->t_tail = tmp->p_id;
				}
			}

			IDTYPE pre = tmp->pre;
			IDTYPE next = tmp->next;
			IDTYPE pre_next = tmp->p_id;
			IDTYPE next_pre = tmp->p_id;


			if (n_pt->containPoint(tmp)) {//这个判断貌似没有必要
			        newnode->data.insert(tmp->p_id);
				point_list.insert(pair<IDTYPE, OctPoint*>(tmp->p_id, tmp));
				//g_PtList.erase(tmp->p_id);
				n_pt->data.erase(tmp->p_id);
			}

            //循环的继续的条件：1、这个点之前有点且在这个leaf中
            //或者 2、这个点之后有点且在这个leaf中
            while ((pre != NONE && n_pt->data.find(pre) != n_pt->data.end())
                    || (next != NONE && n_pt->data.find(next) != n_pt->data.end()))
            {
                //是条件1
                if (pre != NONE && n_pt->data.find(pre) != n_pt->data.end())
                {
                    //pre的时间早于traj_list的头
                    if (comparePointTime(pre,traj_list.find(tmp->p_tid)->second->t_head))
                    {
                        //更新新的trajectory，点的信息是完整的，没有必要更新
                        traj_list.find(tmp->p_tid)->second->t_head = pre;
                    }
                    point_list.insert(
                            pair<IDTYPE, OctPoint*>(pre,
                                g_PtList.find(pre)->second->clone()));

                    newnode->data.insert(pre);

                    n_pt->data.erase(pre);  //为了在此过程中循环需要
					pre_next = pre;
					pre = g_PtList.find(pre)->second->pre; //更新pre标记，与结构无关
					//g_PtList.erase(pre);  //为了发给另一个server
				}
				//是条件2
				if (next != NONE && n_pt->data.find(next) != n_pt->data.end()) {
                    if (comparePointTime(
                                traj_list.find(tmp->p_tid)->second->t_tail, next)) {
                        traj_list.find(tmp->p_tid)->second->t_tail = next;
                    }
                    point_list.insert(
                            pair<IDTYPE, OctPoint*>(next,
                                g_PtList.find(next)->second->clone()));
                    newnode->data.insert(next);

                    n_pt->data.erase(next);  //为了在此过程中循环需要
                    next_pre = next;
                    next = g_PtList.find(next)->second->next; //更新next标记，与结构无关
                    //g_PtList.erase(next);  //为了发给另一个server
				}
			}

			//边界点，另一个点next在在另一个server的Leaf上,需要插值
			if (next != NONE && n_pt->data.find(next) == n_pt->data.end()
       && !containPointNew(g_PtList.find(next)->second,treeNewLow,treeNewHigh) )
                        {
				OctPoint *point;
                point = g_PtList.find(next)->second;
                //point->printIt();
				OctPoint *point_new = new OctPoint();
                OctPoint *point_new_pre = g_PtList.find(next_pre)->second;
				g_ptNewCount++;
				geneBorderPoint(point_new_pre, point, point_new, treeNewLow, treeNewHigh);  //求出与面的交点

				point_new->p_id = -g_ptNewCount;
				point_new->p_tid = point_new_pre->p_tid;                  //补全信息
				tmp->next = point_new->p_id;
				point_new->pre = point_new_pre->p_id;
				point_new->next = NONE;                             //更新相关的next和pre

                //加入到新的hashmap里
				point_list.insert(pair<IDTYPE, OctPoint*>(point_new->p_id, point_new)); 

                newnode->data.insert(point_new->p_id);

				//更新traj
				if (comparePointTime(traj_list.find(tmp->p_tid)->second->t_tail,
                            point_new->p_id))
                {
                    traj_list.find(tmp->p_tid)->second->t_tail = point_new->p_id;
				}
                //traj_list.find(tmp->p_tid)->second->printIt();
			}

            //printNodes();
            //printPoints();
			//边界点，另一个点pre在在另一个server的Leaf上,需要插值
            if (pre != NONE && n_pt->data.find(pre) == n_pt->data.end()
                    && !containPointNew(g_PtList.find(pre)->second,treeNewLow,treeNewHigh) )
            {
				OctPoint *point;
                point = g_PtList.find(pre)->second;

				OctPoint *point_new = new OctPoint();
                OctPoint *point_new_next = g_PtList.find(pre_next)->second;
				g_ptNewCount++;
				geneBorderPoint( point, point_new_next, point_new, treeNewLow, treeNewHigh);  //求出与面的交点

				point_new->p_id = -g_ptNewCount;
				point_new->p_tid = point_new_next->p_tid;                  //补全信息
				tmp->pre = point_new->p_id;
				point_new->next = point_new_next->p_id;
				point_new->pre = NONE;                             //更新相关的next和pre

                //加入到新的hashmap里
				point_list.insert(pair<IDTYPE, OctPoint*>(point_new->p_id, point_new));
                newnode->data.insert(point_new->p_id);

				//更新traj
                if (comparePointTime(point_new->p_id,traj_list.find(tmp->p_tid)->second->t_head))
                {
                    traj_list.find(tmp->p_tid)->second->t_head = point_new->p_id;
                }
            }
		}
	}
	else//非叶子节点的复制
	 {
		OctTNode *newnode = new OctIdxNode(n_pt->n_id,n_pt->n_type,n_pt->n_domLow,n_pt->n_domHigh,n_pt->n_father);
		node_list.insert(pair<int, OctTNode*>(newnode->n_id, newnode));
		for (int i = 0; i < 8; i++) {
			if (n_pt->n_children[i] >= 0) {
				copy(g_NodeList.find(n_pt->n_children[i])->second, treeNewLow, treeNewHigh);
			}
		}
	}

}

bool between(double start, double end, double mid) {
	if ((start >= mid && end <= mid) || (start <= mid && end >= mid))
		return true;
	else
		return false;
}

void OctTree::geneBorderPoint(OctPoint *pt1, OctPoint *pt2,
		OctPoint *result_point, double *low, double *high) {
	double result[3];
	for (int i = 0; i < 3; i++) {
		if (between(pt1->p_xyz[i], pt2->p_xyz[i], high[i])) {
			result[i] = high[i]; //dimention 1
			// (x1-x) / (x -x2) = p1 / p2  => x = (p2*x1+p1*x2)/(p1+p2)
			double p1 = pt1->p_xyz[i] - high[i];
			double p2 = high[i] - pt2->p_xyz[i];
			double x1 = pt1->p_xyz[(i + 1) % 3];
			double x2 = pt2->p_xyz[(i + 1) % 3];
			double y1 = pt1->p_xyz[(i + 2) % 3];
			double y2 = pt2->p_xyz[(i + 2) % 3];
			result[(i + 1) % 3] = (p2 * x1 + p1 * x2) / (p1 + p2); //dimention 2
			result[(i + 2) % 3] = (p2 * y1 + p1 * y2) / (p1 + p2); //dimention 3

			break;
		} else if (between(pt1->p_xyz[i], pt2->p_xyz[i], low[i])) {
			result[i] = low[i];                        //dimention 1
			// (x1-x) / (x -x2) = p1 / p2  => x = (p2*x1+p1*x2)/(p1+p2)
			double p1 = pt1->p_xyz[i] - low[i];
			double p2 = low[i] - pt2->p_xyz[i];
			double x1 = pt1->p_xyz[(i + 1) % 3];
			double x2 = pt2->p_xyz[(i + 1) % 3];
			double y1 = pt1->p_xyz[(i + 2) % 3];
			double y2 = pt2->p_xyz[(i + 2) % 3];
			result[(i + 1) % 3] = (p2 * x1 + p1 * x2) / (p1 + p2); //dimention 2
			result[(i + 2) % 3] = (p2 * y1 + p1 * y2) / (p1 + p2); //dimention 3
			break;
		}
	}
	memcpy(result_point->p_xyz, result, sizeof(double) * 3);
}

int OctTree::pointInWhichNode(OctPoint *pt) {
	OctTNode *root = g_NodeList.find(1)->second;
	//OctTNode *child;
	while (root->n_type != LEAF) {
		for (int i = 0; i < 8; i++) {
			if (0 > root->n_children[i])
				continue;
			//child = g_NodeList.find(root->n_children[i])->second;
			if (root->containPoint(pt, i)) {
				root = g_NodeList.find(root->n_children[i])->second;
				break;
			}
		}

	}
	return root->n_id;

}

int OctTree::recreateTrajs(vector<OctPoint *> pt_vector, hash_map<IDTYPE, Traj*> &trajs) {
    size_t i;
    hash_map<int, OctPoint *> traj_head; 
    hash_map<int, OctPoint *> traj_tail; 
    hash_map<int, OctPoint *>::iterator it; 
    OctPoint *cur_point,*head, *tail;
    for (i = 0; i < pt_vector.size(); i++) {
        cur_point = pt_vector[i];

        it = traj_head.find(cur_point->p_tid);
        if(it == traj_head.end()) {
            traj_head.insert(pair<int, OctPoint *>(cur_point->p_tid, cur_point));
        } else {
            head = it->second;
            if(cur_point->p_xyz[2] < head->p_xyz[2]) {
                it->second = cur_point;
            }
        }

        it = traj_tail.find(cur_point->p_tid);
        if(it == traj_tail.end()) {
            traj_tail.insert(pair<int, OctPoint *>(cur_point->p_tid, cur_point));
        } else {
            tail = it->second;
            if(cur_point->p_xyz[2] > tail->p_xyz[2]) {
                it->second = cur_point;
            }
        }
    }

    hash_map<IDTYPE, Traj*>::iterator tit;
    for(it = traj_head.begin(); it != traj_head.end(); ++it) {
        head = it->second;
        if(head->pre != -1) {
            head = g_PtList.find(head->pre)->second;
        }
        Traj *t = new Traj(head->p_tid, head->p_id, -1);
        trajs.insert(pair<int, Traj*>(t->t_id, t));
    }

    for(it = traj_tail.begin(); it != traj_tail.end(); ++it) {
        tail = it->second;
        if(tail->next != -1) {
            tail = g_PtList.find(tail->next)->second;
        }
        tit = trajs.find(tail->p_tid);
        if(tit == trajs.end()) {
            write_log(ERROR_LOG, "recreate_trajs: trajs find failed.\n");
            return FALSE;
        }
        Traj *t = tit->second;
        t->t_tail = tail->p_id;
    }

    return TRUE;
}


void OctTree::rangeQuery(double *low, double *high, vector<OctPoint*> &pt_vector) {
	OctTNode *root = g_NodeList.find(tree_root)->second;
	root->rangeQueryNode(low, high, pt_vector);
}

// sort by minimum distance
void NNQueryQueue::push(OctTNode *new_node, double dist) {
    NNEntry *entry = new NNEntry(new_node, dist);
    std::vector<NNEntry *>::iterator it;
    for(it = m_queue.begin(); it != m_queue.end(); ++it) {
        if((*it)->m_iDist > entry->m_iDist) {
            break;
        }
    }
    m_queue.insert(it, entry);
}

OctTNode * NNQueryQueue::pop() {
    OctTNode *node= NULL;
    std::vector<NNEntry *>::iterator it = m_queue.begin();
    if(it != m_queue.end()) {
        node = (*it)->m_pNode;
        delete (*it);
        m_queue.erase(it);
    }
    return node;
}

// delete oct node from queue when the distance between the query 
// and this oct node is larger than fm
void NNQueryQueue::deleteOctTNode(double fm) {
    std::vector<NNEntry *>::iterator it;
    for(it = m_queue.begin(); it != m_queue.end();) {
        if((*it)->m_iDist > fm) {
            delete (*it);
            it = m_queue.erase(it);
        } else {
            ++it;
        }
    }
}

int time_overlap(struct min_max_st o_min_max, struct min_max_st c_min_max) {
    // TODO eps set to 0.1
    if(c_min_max.end < o_min_max.start && abs(c_min_max.end - o_min_max.start) > eps) {
        return -1;
    }
    if(c_min_max.start > o_min_max.end && abs(c_min_max.start - o_min_max.end) > eps) {
        return 1;
    }
    return 0;
}

int do_update_fm(double &fm, std::vector<struct min_max_st> &array_min_max, double *low, double *high) {
    std::vector<struct min_max_st>::iterator it = array_min_max.begin();
    if(array_min_max.size() > 1 || array_min_max.size() == 0) {
        return FALSE;
    } else {
        struct min_max_st min_max = array_min_max[0];
        if(abs(min_max.start - low[2]) < eps && abs(min_max.end - high[2]) < eps) {
            if(min_max.dist < fm) {
                fm = array_min_max[0].dist;
                array_min_max.erase(it);
            }
            return TRUE;
        } else {
            return FALSE;
        }
    }
}


void update_fm(double &fm, std::vector<struct min_max_st> &array_min_max, struct min_max_st new_min_max, double *low, double *high) {
    std::vector<struct min_max_st>::iterator it;
    // first enter this function
    if(array_min_max.size() == 0) {
        array_min_max.push_back(new_min_max);
        do_update_fm(fm, array_min_max, low, high);
        return;
    }
    for(it = array_min_max.begin(); it != array_min_max.end();) {
        struct min_max_st min_max = (*it);
        int locate = time_overlap(min_max, new_min_max);
        // new_min_max is on the left side of min_max
        if(-1 == locate) {
            array_min_max.insert(it, new_min_max);
            return;
        } else if(0 == locate){
            /* combine overlaped time span
             * note: new_min_max could overlap with multiple min_max 
             * we combine new_min_max by modify new_min_max and delete current min_max
             * if current min_max is the last element in the array, insert new_min_max
             * otherwise loop continue.
             * */
            // TODO max or min
            new_min_max.dist = new_min_max.dist > min_max.dist ? new_min_max.dist : min_max.dist;
            new_min_max.start = new_min_max.start < min_max.start ? new_min_max.start : min_max.start;
            new_min_max.end = new_min_max.end > min_max.end ? new_min_max.end : min_max.end;
            it = array_min_max.erase(it);
            if(it == array_min_max.end()) {
                array_min_max.push_back(new_min_max);
                do_update_fm(fm, array_min_max, low, high);
                return;
            }
        } else {
            // new_min_max is on the right side of min_max
            if((it + 1) == array_min_max.end()) {
                array_min_max.push_back(new_min_max);
                return;
            } else {
                ++it;
            }
        }
    }
}

// get max distance between query point and given trajectory
struct min_max_st get_max_distance(double *low, double *high, Traj* traj) {
    int i;
    struct min_max_st max_dist;
    double dist;
    OctPoint *p_cur, *p_next;
    struct point start, end, pt;

    IDTYPE pid = traj->t_head;
    max_dist.dist = -1;
    p_cur =  g_PtList.find(pid)->second;
    while(p_cur->next != -1) {
        pid = p_cur->next; 

        p_next = g_PtList.find(pid)->second; 
        // ignore the interpolation point
        while(p_next->p_id < -1) {
            pid = p_next->p_id; 
            p_next = g_PtList.find(pid)->second; 
        }

        // get close to the query time region
        if(p_next->p_xyz[2] < low[2]) {
            // move next
            p_cur = p_next;
            continue;
        }

        if(p_cur->p_xyz[2] > high[2]) {
            break;
        }

        //construct a line segment  
        for(i = 0; i < MAX_DIM_NUM; i++) {
            start.axis[i] = p_cur->p_xyz[i];
            end.axis[i] = p_next->p_xyz[i];
        }

        // calc the intersection point between start/end time of query and current trajs
        double z;
        if(low[2] > start.axis[2] && low[2] < end.axis[2] ) {
            z = low[2];
            pt.axis[0] = ((z - start.axis[2]) * end.axis[0] + (end.axis[2] - z) * start.axis[0]) / \
                             (end.axis[2] - start.axis[2]);
            pt.axis[1] = ((z - start.axis[2]) * end.axis[1] + (end.axis[2] - z) * start.axis[1]) / \
                             (end.axis[2] - start.axis[2]);
            // here the x-axis and y-axis of low and high are the same
            // calc distance between query point and intersection point at time start
            dist = sqrt((pt.axis[0] - low[0])*(pt.axis[0] - low[0]) + (pt.axis[1] - low[1]) * (pt.axis[1] - low[1]));

            // update start time stamp of max_dist
            max_dist.start = low[2];

        } else if (high[2] > start.axis[2] && high[2] < end.axis[2]) {
            z = high[2];
            pt.axis[0] = ((z - start.axis[2]) * end.axis[0] + (end.axis[2] - z) * start.axis[0]) / \
                             (end.axis[2] - start.axis[2]);
            pt.axis[1] = ((z - start.axis[2]) * end.axis[1] + (end.axis[2] - z) * start.axis[1]) / \
                             (end.axis[2] - start.axis[2]);
            // calc distance between query point and intersection point at time end 
            dist = sqrt((pt.axis[0] - low[0])*(pt.axis[0] - low[0]) + (pt.axis[1] - low[1]) * (pt.axis[1] - low[1]));

            // update end time stamp of max_dist
            max_dist.end = high[2];
        } else {
            // traj inside query
            // calc distance between query point and traj point
            double dist1, dist2;
            dist1 = sqrt((start.axis[0] - low[0])*(start.axis[0] - low[0]) + (start.axis[1] - low[1]) * (start.axis[1] - low[1]));
            dist2 = sqrt((end.axis[0] - low[0])*(end.axis[0] - low[0]) + (end.axis[1] - low[1]) * (end.axis[1] - low[1]));
            dist = dist1 > dist2 ? dist1 : dist2;
        }
        if(dist > max_dist.dist) {
            max_dist.dist = dist;
        }

        // if the start point of traj is inside the query region update start time stamp of max_dist
        if(pid == traj->t_head) {
            max_dist.start = start.axis[2];
        } 

        // if the end point of traj is inside the query region update end time stamp of max_dist
        if(p_next->p_id == traj->t_tail) {  
            // update end time stamp of max_dist
            max_dist.end = end.axis[2];
        } 

        // move next
        p_cur = p_next;
    }
    return max_dist;
}

/*struct min_max_st get_min_max_distance(double *low, double *high, hash_map<IDTYPE, Traj*> &new_trajs) {
    struct min_max_st cur_min_max;
    hash_map<IDTYPE, Traj*>::iterator tit;
    // get the traj that neareast to the query point
    for(tit = new_trajs.begin(); tit != new_trajs.end(); ++tit) {
        Traj *traj = tit->second;
        cur_dist = get_max_distance(low, high, traj);
        if(cur_dist.dist < min_max_dist.dist) {
            min_max_dist = cur_dist;
        }
    }
    return min_max_dist;
} */

// combine new_trajs with the trajs
// trajs: current results of whole query
// new_trajs: results in current oct node(leaf)
void append_trajs(double *low, double *high, double fm, hash_map<IDTYPE, Traj*> new_trajs,hash_map<IDTYPE, Traj*> &trajs) {
    hash_map<IDTYPE, Traj*>::iterator it;
    for(it = new_trajs.begin(); it != new_trajs.end(); ++it) {
        IDTYPE t_id = it->first;
        Traj *traj = it->second;
        if(trajs.find(t_id) == trajs.end()) {
            trajs.insert(pair<IDTYPE, Traj*>(t_id, traj));
        }
    }
}

// non-recursive version nn query
void OctTree::nearestNeighborQuery(double *low, double *high, hash_map<IDTYPE, Traj*> &trajs) {
    int i;
    struct min_max_st cur_min_max;
    hash_map<IDTYPE, Traj*>::iterator tit;

    // set to infinity at first
    double fm = DBL_MAX;
    // sort by time start
    std::vector<struct min_max_st> array_min_max;
    NNQueryQueue queue;
	OctTNode *root = g_NodeList.find(tree_root)->second;
    queue.push(root, root->getMinimumDistance(low, high));
    while(!queue.empty()) {
        OctTNode *node = queue.pop();
        if(node->n_type == LEAF) {
            hash_map<IDTYPE, Traj*> new_trajs;
            OctLeafNode *leaf = dynamic_cast<OctLeafNode*>(node);
            leaf->retrieveTrajs(low, high, fm, new_trajs);
            if(new_trajs.size() > 0) {
                double old_fm = fm;
                // for each traj calc max distance and update fm
                for(tit = new_trajs.begin(); tit != new_trajs.end(); ++tit) {
                    Traj *traj = tit->second;
                    cur_min_max = get_max_distance(low, high, traj);
                    update_fm(fm, array_min_max, cur_min_max, low, high); 
                }
                if(old_fm > fm){
                    queue.deleteOctTNode(fm);
                }
                append_trajs(low, high, fm, new_trajs, trajs);
            }
        } else {
            for(i = 0; i < 8; i++) {
                if (node->n_children[i] == -1)
                    continue;
                OctTNode *tmp = g_NodeList.find(node->n_children[i])->second;
                queue.push(tmp, tmp->getMinimumDistance(low, high));
            }
        }
    }
}

void OctTree::NNQuery(double *low,double *high,vector<OctPoint*> &pt_vector) {
	OctTNode *root = g_NodeList.find(tree_root)->second;
	root->NNQueryNode(low, high, pt_vector);
}
