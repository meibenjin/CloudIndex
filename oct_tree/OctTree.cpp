#include "OctTree.h"
#include "utils.h"

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
						pt_vector[i]->next == 0                         //next为空
								) {
					OctPoint *point_new = new OctPoint();
					g_ptNewCount++;
					geneBorderPoint(pt_vector[i], pt, point_new);      //求出与面的交点

					point_new->p_id = -g_ptNewCount;
					point_new->p_tid = pt->p_tid;                  //补全信息
					pt->pre = point_new->p_id;
					point_new->next = pt->p_id;
					point_new->pre = 0;                          //更新相关的next和pre

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
	int x_axis[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };
	int y_axis[8] = { 0, 1, 4, 5, 2, 3, 6, 7 };
	int z_axis[8] = { 0, 2, 4, 6, 1, 3, 5, 7 };
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
    FILE *fp;
    char buffer[1024];
    fp = fopen(RESULT_LOG, "ab+");
    if (fp == NULL) {
        printf("log: open file %s error.\n", RESULT_LOG);
        return;
    }
    strcpy(buffer, "leaf node split here 1\n");
    fwrite(buffer, strlen(buffer), 1, fp);
    fclose(fp);
	//find which axis to cut
	int x_axis[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };
	int y_axis[8] = { 0, 1, 4, 5, 2, 3, 6, 7 };
	int z_axis[8] = { 0, 2, 4, 6, 1, 3, 5, 7 };
	double low[3];
	double high[3];
	int condition;
	setDom(low, high, getLow, condition);                         //new tree dom
	OctTNode *root = new OctTNode(1, IDX, tree_domLow, tree_domHigh, -1);
	//OctTNode *now = g_NodeList.find(tree_root)->second;
	for (int i = 0; i < 8; i++) {
		root->n_children[i] = now->n_children[i];
	}

    fp = fopen(RESULT_LOG, "ab+");
    if (fp == NULL) {
        printf("log: open file %s error.\n", RESULT_LOG);
        return;
    }
    strcpy(buffer, "leaf node split here 2\n");
    fwrite(buffer, strlen(buffer), 1, fp);
    fclose(fp);

	for (int j = 0; j < 4; j++) {
		switch (condition) {
		case 0:
			if (!getLow)
			{
				root->n_children[x_axis[j]] = -1;
			}

			else
			{
				root->n_children[x_axis[j + 4]] = -1;
			}

			break;
		case 1:
			if (!getLow)
			{
				root->n_children[y_axis[j]] = -1;
			}
			else
			{
				root->n_children[y_axis[j + 4]] = -1;
			}
			break;
		default:
			if (!getLow)
			{
				root->n_children[z_axis[j]] = -1;
			}
			else
			{
				root->n_children[z_axis[j + 4]] = -1;
			}
			break;
		}
	}

	for (int i = 0; i < 8; i++) {
		if (-1 != root->n_children[i]) {
			root->n_ptCount +=
					g_NodeList.find(root->n_children[i])->second->n_ptCount;
		}
	}

	node_list.insert(pair<int, OctTNode*>(1, root));

    fp = fopen(RESULT_LOG, "ab+");
    if (fp == NULL) {
        printf("log: open file %s error.\n", RESULT_LOG);
        return;
    }
    strcpy(buffer, "leaf node split here 3\n");
    fwrite(buffer, strlen(buffer), 1, fp);
    fclose(fp);

	copy(root,getLow);


    fp = fopen(RESULT_LOG, "ab+");
    if (fp == NULL) {
        printf("log: open file %s error.\n", RESULT_LOG);
        return;
    }
    strcpy(buffer, "leaf node split here 4\n");
    fwrite(buffer, strlen(buffer), 1, fp);
    fclose(fp);

}

bool comparePointTime(IDTYPE p1, IDTYPE p2) {
	OctPoint *point1, *point2;
	if (g_PtList.find(p1) != g_PtList.end()) {
		point1 = g_PtList.find(p1)->second;
	} else {
		point1 = point_list.find(p1)->second;
	}
	point2 = g_PtList.find(p2)->second;

	if (point1->p_xyz[2] < point2->p_xyz[2]) {
		return true;
	} else
		return false;
}

void OctTree::copy(OctTNode *n_pt,double *treeNewLow,double *treeNewHigh) {
    FILE *fp;
    char buffer[1024];
	if (n_pt->n_type == LEAF) {//叶子节点的复制
                //新的server上树的范围
		//OctTNode *tmp = g_NodeList.at(root->n_id);
		OctTNode *newnode = new OctLeafLode(n_pt->n_id,n_pt->n_type,n_pt->n_domLow,n_pt->n_domHigh,n_father);
		newnode->n_ptCount = n_pt->n_ptCount;

		node_list.insert(pair<int, OctTNode*>(newnode->n_id, newnode)); //增加到新的server的hash_map上
		//g_NodeList.erase(n_pt->n_id);                   //本机server上NodeList相应的删除

		while (!n_pt->data.empty()) {//针对这个叶子节点中的data域，按每个Traj的顺序
			hash_set<IDTYPE>::iterator top = n_pt->data.begin();
			OctPoint *tmp = g_PtList.find(*top)->second;

                        //下面的if...else主要为了更新在新的server上这个点所在的Traj信息
                        //如果traj_list没有这个点所在的traj,在新的server上新建一个头,尾为当前Piont的Traj
			if (traj_list.find(tmp->p_tid) == traj_list.end()) {
				Traj *trajectory = new Traj(tmp->p_tid, tmp->p_id, tmp->p_id);
				traj_list.insert(pair<IDTYPE, Traj*>(tmp->p_tid, trajectory));
			}
			else//出现过
			 {
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

			while ((pre != 0 && n_pt->data.find(pre) != n_pt->data.end())
					|| (next != 0 && n_pt->data.find(next) != n_pt->data.end()))
                        {
                                //是条件1
				if (pre != 0 && n_pt->data.find(pre) != n_pt->data.end())
				{
				        //pre的时间早于traj_list的头
					if (comparePointTime(pre,traj_list.find(tmp->p_tid)->second->t_head))
					{
					        //更新新的trajectory，点的信息是完整的，没有必要更新
						traj_list.find(tmp->p_tid)->second->t_head = pre;
					}
					point_list.insert(
							pair<IDTYPE, OctPoint*>(pre,
									g_PtList.find(pre)->second));

                                        newnode->data.insert(pre);

					pre_next = pre;
					pre = g_PtList.find(pre)->second->pre; //更新pre标记，与结构无关
					//g_PtList.erase(pre);  //为了发给另一个server
					n_pt->data.erase(pre);  //为了在此过程中循环需要
				}
				//是条件2
				if (next != 0 && n_pt->data.find(next) != n_pt->data.end()) {
					if (comparePointTime(
							traj_list.find(tmp->p_tid)->second->t_tail, next)) {
						traj_list.find(tmp->p_tid)->second->t_tail = next;
					}
					point_list.insert(
							pair<IDTYPE, OctPoint*>(next,
									g_PtList.find(next)->second));
                                        newnode->data.insert(next);
					next_pre = next;
					next = g_PtList.find(next)->second->next; //更新next标记，与结构无关
					//g_PtList.erase(next);  //为了发给另一个server
					n_pt->data.erase(next);  //为了在此过程中循环需要

				}
			}
			//边界点，另一个点next在在另一个server的Leaf上,需要插值
			if (next != 0 && n_pt->data.find(next) == n_pt->data.end()
       && !containPointNew(g_PtList.find(next)->second,treeNewLow,treeNewHigh) )
                        {
				OctPoint *point;
				//if (g_PtList.find(next) != g_PtList.end()) {
                                point = g_PtList.find(next)->second;
				//} else {
				//	point = point_list.find(next)->second;
				//}

				OctPoint *point_new = new OctPoint();
				g_ptNewCount++;
				geneBorderPoint(tmp, point, point_new);  //求出与面的交点

				point_new->p_id = -g_ptNewCount;
				point_new->p_tid = tmp->p_tid;                  //补全信息
				tmp->next = point_new->p_id;
				point_new->pre = tmp->p_id;
				point_new->next = 0;                             //更新相关的next和pre

				point_list.insert(
						pair<IDTYPE, OctPoint*>(point_new->p_id, point_new)); //加入到新的hashmap里
                                newnode->data.insert(point_new);
				//更新traj
				if (comparePointTime(traj_list.find(tmp->p_tid)->second->t_tail,
						point_new->p_id))
                                {
					traj_list.find(tmp->p_tid)->second->t_tail =
							point_new->p_id;
				}
                                /*
				//下面将这个新点加入到相邻的leaf node里
				int point_in_which = pointInWhichNode(point); //next节点在哪一个leaf node中
				OctTNode *next_leaf;
				//bool flag_new;
				if (g_NodeList.find(point_in_which) != g_NodeList.end()) //判断这个node是否是留在本server的
                                {
					next_leaf = g_NodeList.find(point_in_which)->second;

					next_leaf->data.insert(point_new->p_id);

					OctPoint *next_point = g_PtList.find(next)->second;

					//以下三行，新建了一个与插入的点相同的点，但其next指向下一个leaf node里的point (next_point)
					OctPoint *point_new_next = new OctPoint(point_new);
					point_new_next->next = next_point->p_id;
					g_PtList.insert(
							pair<IDTYPE, OctPoint*>(point_new_next->p_id,
									point_new_next));

					next_point->pre = point_new_next->p_id;

					g_TrajList.find(next_point->p_tid)->second->t_head =
							point_new_next->p_id;

				} else
				{
					next_leaf = node_list.find(point_in_which)->second;
					next_leaf->data.insert(point_new->p_id);

					OctPoint *next_point = point_list.find(next)->second;

					//以下三行，新建了一个与插入的点相同的点，但其next指向下一个leaf node里的point (next_point)
					OctPoint *point_new_next = new OctPoint(point_new);
					point_new_next->next = next_point->p_id;
					point_list.insert(
							pair<IDTYPE, OctPoint*>(point_new_next->p_id,
									point_new_next));

					next_point->pre = point_new_next->p_id;

					traj_list.find(next_point->p_tid)->second->t_head =
							point_new_next->p_id;
				}
				*/
			}
			//边界点，另一个点pre在在另一个server的Leaf上,需要插值
                        if (pre != 0 && n_pt->data.find(pre) == n_pt->data.end()
       && !containPointNew(g_PtList.find(pre)->second,treeNewLow,treeNewHigh) )
                        {
				OctPoint *point;
				//if (g_PtList.find(next) != g_PtList.end()) {
                                point = g_PtList.find(pre)->second;
				//} else {
				//	point = point_list.find(next)->second;
				//}

				OctPoint *point_new = new OctPoint();
				g_ptNewCount++;
				geneBorderPoint( point, tmp,point_new);  //求出与面的交点

				point_new->p_id = -g_ptNewCount;
				point_new->p_tid = tmp->p_tid;                  //补全信息
				tmp->pre = point_new->p_id;
				point_new->next = tmp->p_id;
				point_new->pre = 0;                             //更新相关的next和pre

				point_list.insert(
						pair<IDTYPE, OctPoint*>(point_new->p_id, point_new)); //加入到新的hashmap里
                                newnode->data.insert(point_new);
				//更新traj
				if (comparePointTime(point_new->p_id,traj_list.find(tmp->p_tid)->second->head))
                                {
					traj_list.find(tmp->p_tid)->second->t_head =
							point_new->p_id;
				}
                        }
		}
        fp = fopen(RESULT_LOG, "ab+");
        if (fp == NULL) {
            printf("log: open file %s error.\n", RESULT_LOG);
            return;
        }
        strcpy(buffer, "copy leaf node here 2\n");
        fwrite(buffer, strlen(buffer), 1, fp);
        fclose(fp);
	}
	else//非叶子节点的复制
	 {
        fp = fopen(RESULT_LOG, "ab+");
        if (fp == NULL) {
            printf("log: open file %s error.\n", RESULT_LOG);
            return;
        }
        strcpy(buffer, "copy leaf node here 3\n");
        fwrite(buffer, strlen(buffer), 1, fp);
        fclose(fp);
		//OctTNode *tmp = g_NodeList.at(root->n_id);
		node_list.insert(pair<int, OctTNode*>(n_pt->n_id, n_pt));
		for (int i = 0; i < 8; i++) {
			if (n_pt->n_children[i] != -1) {
				copy(g_NodeList.find(n_pt->n_children[i])->second);
			}
		}
	}

}
/*
 void OctTree::insertBetweenServer(){
 hash_map<IDTYPE,Traj*>::iterator ite;
 for(ite=g_TrajList.begin(); ite!= g_TrajList.end(); ite++){
 vector<OctPoint*>  pt_vector;
 IDTYPE endpoint = ite->second->t_tail;
 OctPoint *pt = g_PtList.find(endpoint)->second;
 double range[3] = {1.0,1.0,1.0};//需要定义一下，以这个点为中心的一个范围
 double low[3] ,high[3];
 for(int i = 0; i<3 ; i++){
 low[i] = pt->p_xyz[i] - range[i];
 high[i] = pt->p_xyz[i] + range[i];
 }
 //benjin rangequery
 rangeQuery(low,high,pt_vector);  //得到包含与其相邻点的vector   Benjin

 for(int i=0 ; i<pt_vector.size() ; i++){
 if( pt_vector[i]->p_tid == pt->p_tid  &&   //相同Traj
 pt_vector[i]->p_xyz[2] > pt->p_xyz[2] && //在其后
 pt_vector[i]->pre ==0                              //pre为空
 )
 {
 OctPoint *point_new = new OctPoint();
 g_ptNewCount++;
 geneBorderPoint(pt, pt_vector[i],point_new);//求出与面的交点

 point_new->p_id = -g_ptNewCount;
 point_new->p_tid = pt->p_tid;                  //补全信息
 pt->next = point_new->p_id;
 point_new->pre = pt->p_id;
 point_new->next = 0;                                  //更新相关的next和pre

 ite->second->t_tail = point_new->p_id ; //更新所在Traj的tail
 int nid = pointInWhichNode(point_new);
 g_NodeList.find(nid)->second->data.insert(point_new->p_id);
 //插入到对应的节点的data域里
 g_PtList.insert(pair<IDTYPE,OctPoint*>(point_new->p_id,point_new));
 //插入到全局PtList中
 // TODO (liudehai#1#): 本金将这个点发到邻居server，邻居server data域里 并 更新所在Traj的head
 }
 }
 }



 }
 */
bool between(double start, double end, double mid) {
	if ((start >= mid && end <= mid) || (start <= mid && end >= mid))
		return true;
	else
		return false;
}

void OctTree::geneBorderPoint(OctPoint *pt1, OctPoint *pt2,
		OctPoint *result_point) {
	double result[3];
	for (int i = 0; i < 3; i++) {
		if (between(pt1->p_xyz[i], pt2->p_xyz[i], tree_domHigh[i])) {
			result[i] = tree_domHigh[i]; //dimention 1
			// (x1-x) / (x -x2) = p1 / p2  => x = (p2*x1+p1*x2)/(p1+p2)
			double p1 = pt1->p_xyz[i] - tree_domHigh[i];
			double p2 = tree_domHigh[i] - pt1->p_xyz[i];
			double x1 = pt1->p_xyz[(i + 1) % 3];
			double x2 = pt2->p_xyz[(i + 1) % 3];
			double y1 = pt1->p_xyz[(i + 2) % 3];
			double y2 = pt2->p_xyz[(i + 2) % 3];
			result[(i + 1) % 3] = (p2 * x1 + p1 * x2) / (p1 + p2); //dimention 2
			result[(i + 2) % 3] = (p2 * y1 + p1 * y2) / (p1 + p2); //dimention 3

			break;
		} else if (between(pt1->p_xyz[i], pt2->p_xyz[i], tree_domLow[i])) {
			result[i] = tree_domLow[i];                        //dimention 1
			// (x1-x) / (x -x2) = p1 / p2  => x = (p2*x1+p1*x2)/(p1+p2)
			double p1 = pt1->p_xyz[i] - tree_domLow[i];
			double p2 = tree_domLow[i] - pt1->p_xyz[i];
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
			if (-1 == root->n_children[i])
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

void OctTree::rangeQuery(double *low, double *high,
		vector<OctPoint*> &pt_vector) {

	OctTNode *root = g_NodeList.find(tree_root)->second;
	root->rangeQueryNode(low, high, pt_vector);
}
