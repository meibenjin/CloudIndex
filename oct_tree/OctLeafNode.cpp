#include "OctTree.h"

OctLeafNode::OctLeafNode(int nid, NodeType type, double* low, double* high,
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

void OctLeafNode::nodeInsert(OctPoint *pt) {
	//g_PtList.insert(make_pair(pt->p_id,pt));
	g_PtList.insert(pair<int, OctPoint*>(pt->p_id, pt));
	data.insert(pt->p_id);
	n_ptCount++;
	nodeAjust();
}

void OctLeafNode::geneChildRelativeLocation(OctPoint *pt, int* l) {
	double mid[3];
	mid[0] = (n_domLow[0] + n_domHigh[0]) / 2.0;
	mid[1] = (n_domLow[1] + n_domHigh[1]) / 2.0;
	mid[2] = (n_domLow[2] + n_domHigh[2]) / 2.0;

	l[0] = pt->p_xyz[0] > mid[0] ? 1 : -1;
	l[1] = pt->p_xyz[1] > mid[1] ? 1 : -1;
	l[2] = pt->p_xyz[2] > mid[2] ? 1 : -1;

	if (l[2] < 0) {
		if (l[1] < 0) {
			if (l[0] < 0)
				l[3] = 0;
			else
				l[3] = 1;
		} else {
			if (l[0] < 0)
				l[3] = 2;
			else
				l[3] = 3;
		}
	} else {
		if (l[1] < 0) {
			if (l[0] < 0)
				l[3] = 4;
			else
				l[3] = 5;
		} else {
			if (l[0] < 0)
				l[3] = 6;
			else
				l[3] = 7;
		}
	}

}

void OctLeafNode::geneChildRelativeLocation(double* xyz, int* l) {
	double mid[3];
	mid[0] = (n_domLow[0] + n_domHigh[0]) / 2.0;
	mid[1] = (n_domLow[1] + n_domHigh[1]) / 2.0;
	mid[2] = (n_domLow[2] + n_domHigh[2]) / 2.0;
	l[0] = xyz[0] > mid[0] ? 1 : -1;
	l[1] = xyz[1] > mid[1] ? 1 : -1;
	l[2] = xyz[2] > mid[2] ? 1 : -1;

	if (l[2] < 0) {
		if (l[1] < 0) {
			if (l[0] < 0)
				l[3] = 0;
			else
				l[3] = 1;
		} else {
			if (l[0] < 0)
				l[3] = 2;
			else
				l[3] = 3;
		}
	} else {
		if (l[1] < 0) {
			if (l[0] < 0)
				l[3] = 4;
			else
				l[3] = 5;
		} else {
			if (l[0] < 0)
				l[3] = 6;
			else
				l[3] = 7;
		}
	}

}

int OctLeafNode::geneChildRelativeLocation(IDTYPE pid) {
	int *l = new int[3];
	double *mid = new double[3];
	OctPoint *pt = g_PtList.find(pid)->second;
	mid[0] = (n_domLow[0] + n_domHigh[0]) / 2.0;
	mid[1] = (n_domLow[1] + n_domHigh[1]) / 2.0;
	mid[2] = (n_domLow[2] + n_domHigh[2]) / 2.0;
	l[0] = pt->p_xyz[0] > mid[0] ? 1 : -1;
	l[1] = pt->p_xyz[1] > mid[1] ? 1 : -1;
	l[2] = pt->p_xyz[2] > mid[2] ? 1 : -1;

	if (l[2] < 0) {
		if (l[1] < 0) {
			if (l[0] < 0)
				return 0;
			else
				return 1;
		} else {
			if (l[0] < 0)
				return 2;
			else
				return 3;
		}
	} else {
		if (l[1] < 0) {
			if (l[0] < 0)
				return 4;
			else
				return 5;
		} else {
			if (l[0] < 0)
				return 6;
			else
				return 7;
		}
	}

}

int OctLeafNode::geneChildRelativeLocation(double* xyz) {
	int *l = new int[3];
	double *mid = new double[3];
	mid[0] = (n_domLow[0] + n_domHigh[0]) / 2.0;
	mid[1] = (n_domLow[1] + n_domHigh[1]) / 2.0;
	mid[2] = (n_domLow[2] + n_domHigh[2]) / 2.0;
	l[0] = xyz[0] > mid[0] ? 1 : -1;
	l[1] = xyz[1] > mid[1] ? 1 : -1;
	l[2] = xyz[2] > mid[2] ? 1 : -1;

	if (l[2] < 0) {
		if (l[1] < 0) {
			if (l[0] < 0)
				return 0;
			else
				return 1;
		} else {
			if (l[0] < 0)
				return 2;
			else
				return 3;
		}
	} else {
		if (l[1] < 0) {
			if (l[0] < 0)
				return 4;
			else
				return 5;
		} else {
			if (l[0] < 0)
				return 6;
			else
				return 7;
		}
	}

}

void OctLeafNode::splitPointIntoChild(OctPoint *pt, int child_id) {
	if (-1 == n_children[child_id]) {
		int nid = g_nodeCount + 1;
		NodeType type = LEAF;
		double low[3];
		double high[3];
		octsplit(low, high, child_id, n_domLow, n_domHigh);
		int father = n_id;

		OctLeafNode *tmp = new OctLeafNode(nid, type, low, high, father);

		//g_NodeList.insert(make_pair(nid,tmp));//add to global data structure
		g_NodeList.insert(pair<int, OctTNode*>(nid, tmp));
		n_children[child_id] = nid;

		g_leafCount++;
		g_nodeCount++;
	}
	OctTNode *tmp = g_NodeList.find(n_children[child_id])->second;  //pull out
	tmp->nodeInsert(pt);

	//g_NodeList.insert(make_pair(tmp->n_id,tmp));  //write back

}

int OctLeafNode::judgeBetween(IDTYPE pid1, IDTYPE pid2) {
	int *l1 = new int(3);
	int *l2 = new int(3);
	geneChildRelativeLocation(g_PtList.find(pid1)->second, l1);
	geneChildRelativeLocation(g_PtList.find(pid2)->second, l2);
	int condition = 0;
	for (int i = 0; i < 3; i++) {
		if (l1[i] != l2[i])
			condition++;
	}
	//delete l1,l2;
	return condition;

}
int OctLeafNode::judgeBetween(double* pid1, double* pid2) {
	int l1[3];
	int l2[3];
	geneChildRelativeLocation(pid1, l1);
	geneChildRelativeLocation(pid2, l2);
	int condition = 0;
	for (int i = 0; i < 3; i++) {
		if (l1[i] != l2[i])
			condition++;
	}
	return condition;

}

void OctLeafNode::calMid(double* low, double* high, double* xyz) {
	for (int i = 0; i < 3; i++) {
		xyz[i] = (low[i] + high[i]) / 2.0;
	}
}

void OctLeafNode::geneMidpoint(IDTYPE pid1, IDTYPE pid2, int condition) {
	int l1 = geneChildRelativeLocation(pid1);
	int l2 = geneChildRelativeLocation(pid2);
	double time = (g_PtList.find(pid1)->second->p_time
			+ g_PtList.find(pid2)->second->p_time) / 2.0;
	IDTYPE tid = g_PtList.find(pid1)->second->p_tid;
	double add[3];
	if (condition == 1 || condition == 0) {
		return;
	} else if (condition == 2) {
		calMid(g_PtList.find(pid1)->second->p_xyz,
				g_PtList.find(pid2)->second->p_xyz, add);
		int tmpChild = geneChildRelativeLocation(add);
		int tmp_count = 0;
		while (tmpChild == l1 || tmpChild == l2) { //binary search midpoint
			if (tmp_count++ > 100)
				break;
			if (tmpChild == l1) {
				calMid(add, g_PtList.find(pid2)->second->p_xyz, add);
			} else
				calMid(g_PtList.find(pid1)->second->p_xyz, add, add);
			tmpChild = geneChildRelativeLocation(add);
		}
		IDTYPE pid = -(g_ptNewCount + 1);
		OctPoint *pt = new OctPoint(pid, time, add, tid, pid1, pid2);
		//g_PtList.insert(make_pair(pid,pt));
		g_PtList.insert(pair<int, OctPoint*>(pid, pt));

		g_ptNewCount++;

		int child_id = geneChildRelativeLocation(pid);
		if (-1 == n_children[child_id]) {
			int nid = g_nodeCount + 1;
			NodeType type = LEAF;
			double low[3];
			double high[3];
			octsplit(low, high, child_id, n_domLow, n_domHigh);
			int father = n_id;

			OctLeafNode *tmp = new OctLeafNode(nid, type, low, high, father);

			//g_NodeList.insert(make_pair(nid,tmp));//add to global data structure
			g_NodeList.insert(pair<int, OctTNode*>(nid, tmp));

			n_children[child_id] = nid;

			g_leafCount++;
			g_nodeCount++;
		}

		OctTNode *tmp = g_NodeList.find(n_children[child_id])->second;
		tmp->nodeInsert(pt);
		///g_NodeList.insert(make_pair(tmp->n_id,tmp));
		//��Ӧ���ӽڵ�n_count���һ

	} else if (condition == 3) {
		calMid(g_PtList.find(pid1)->second->p_xyz,
				g_PtList.find(pid2)->second->p_xyz, add);
		int tmpChild = geneChildRelativeLocation(add);
		int tmp_count = 0;
		while (tmpChild == l1 || tmpChild == l2) { //binary search midpoint
			if (tmp_count++ > 100)
				break;
			if (tmpChild == l1) {
				calMid(add, g_PtList.find(pid2)->second->p_xyz, add);
				// 				int condition_new = judgeBetween(add,g_PtList.find(pid2).p_xyz);
				// 				geneMidpoint(add,g_PtList.at(pid2).p_xyz,condition_new);
			} else {
				calMid(g_PtList.find(pid1)->second->p_xyz, add, add);
				// 				int condition_new = judgeBetween(g_PtList.at(pid2).p_xyz,add);
				// 				geneMidpoint(g_PtList.at(pid2).p_xyz,condition_new,add);
			}
			tmpChild = geneChildRelativeLocation(add);
		}
		IDTYPE pid_add = -(g_ptNewCount + 1);
		OctPoint *pt = new OctPoint(pid_add, time, add, tid, pid1, pid2);
		//g_PtList.insert(make_pair(pid_add,pt));
		g_PtList.insert(pair<int, OctPoint*>(pid_add, pt));

		g_ptNewCount++;

		int child_id = geneChildRelativeLocation(pid_add);
		if (-1 == n_children[child_id]) {
			int nid = g_nodeCount + 1;
			NodeType type = LEAF;
			double low[3];
			double high[3];
			octsplit(low, high, child_id, n_domLow, n_domHigh);
			int father = n_id;

			OctLeafNode *tmp = new OctLeafNode(nid, type, low, high, father);

			//g_NodeList.insert(make_pair(nid,tmp));//add to global data structure
			g_NodeList.insert(pair<int, OctTNode*>(nid, tmp));

			n_children[child_id] = nid;

			g_leafCount++;
			g_nodeCount++;
		}

		OctTNode *tmp = g_NodeList.find(n_children[child_id])->second;
		tmp->nodeInsert(pt);

		//g_NodeList.insert(make_pair(tmp->n_id,tmp));
		//��Ӧ���ӽڵ�n_count���һ

		geneMidpoint(pid1, pid_add, judgeBetween(pid1, pid_add));
		geneMidpoint(pid_add, pid2, judgeBetween(pid_add, pid2));

	} else
		cout << "error in condition!" << endl;

}
;
//iterative get the point that added to the new leaf node

IDTYPE OctLeafNode::whichIsCloser(IDTYPE pid1, IDTYPE pid2, IDTYPE pid) {
	if (abs(
			g_PtList.find(pid1)->second->p_xyz[0]
					- g_PtList.find(pid)->second->p_xyz[0])
			> abs(
					g_PtList.find(pid2)->second->p_xyz[0]
							- g_PtList.find(pid)->second->p_xyz[0]))
		return pid2;
	else
		return pid1;
}

bool OctLeafNode::split_or_not() {
	hash_set<IDTYPE>::iterator ite;
	int point_count_in_children[8] = { 0 };
	for (ite = data.begin(); ite != data.end(); ite++) {
		int location = geneChildRelativeLocation(*ite);
		point_count_in_children[location]++;
	}
	for (int i = 0; i < 8; i++) {
		if (threshold
				<= ((double) point_count_in_children[i])
						/ ((double) n_ptCount)) {
			return false;
		}
	}
	return true;
}

void OctLeafNode::nodeSplit() {
	/* ����û��trajectory�Ĵ���
	 while(!data.empty()){
	 hash_set<IDTYPE>::iterator top;
	 top = data.begin();
	 int top_in_which_child = geneChildRelativeLocation(*top);
	 splitPointIntoChild(g_PtList.at(*top),top_in_which_child);
	 data.erase(*top);
	 }
	 convertToIndex();
	 */

	//if(!split_or_not())return;
	while (!data.empty()) {
		hash_set<IDTYPE>::iterator top;
		top = data.begin();

		int top_in_which_child = geneChildRelativeLocation(*top);

		splitPointIntoChild(g_PtList.find(*top)->second, top_in_which_child);

		IDTYPE pre = g_PtList.find(*top)->second->pre;
		IDTYPE next = g_PtList.find(*top)->second->next;
		IDTYPE pre_next = *top;
		IDTYPE next_pre = *top;
		data.erase(*top);
		bool bool_pre, bool_next;
		while (((pre != 0) && data.find(pre) != data.end() && (bool_pre =
				containPoint(g_PtList.find(pre)->second, top_in_which_child)))
				|| ((next != 0) && data.find(next) != data.end()
						&& (bool_next = containPoint(
								g_PtList.find(next)->second, top_in_which_child))))	//point in the same trajectory
		{
			if (pre != 0 && data.find(pre) != data.end()
					&& (bool_pre = containPoint(g_PtList.find(pre)->second,
							top_in_which_child))) {
				//add
				splitPointIntoChild(g_PtList.find(pre)->second,
						top_in_which_child);
				pre_next = pre;
				pre = g_PtList.find(pre)->second->pre;

				//g_PtList.at(g_PtList.at(pre_next)->next)->pre = 0;

				data.erase(pre_next);
			}
			if (next != 0 && data.find(next) != data.end()
					&& (bool_next = containPoint(g_PtList.find(next)->second,
							top_in_which_child))) {
				//add
				splitPointIntoChild(g_PtList.find(next)->second,
						top_in_which_child);
				next_pre = next;
				next = g_PtList.find(next)->second->next;

				//g_PtList.at(g_PtList.at(next_pre)->pre)->next = 0;

				data.erase(next_pre);
			}
		}
		//judge pre,pre_next
		//next,next,pre    intersect?
		int condition;
		if (pre != 0 && data.find(pre) != data.end() && (condition =
				judgeBetween(pre, pre_next)) > 1) {
			geneMidpoint(pre, pre_next, condition);
			data.erase(pre_next);
		}
		if (next != 0 && data.find(next) != data.end() && (condition =
				judgeBetween(next_pre, next)) > 1) {
			geneMidpoint(next_pre, next, condition);
			data.erase(next_pre);
		}
	}
	convertToIndex();

}
;

void OctLeafNode::nodeAjust() {
	if (n_ptCount <= nodeLimit) {
		return;
	} else {
		nodeSplit();
	}
}
;

void OctLeafNode::convertToIndex() {
	//use the same n_id
	OctIdxNode *newnode = new OctIdxNode(n_id, IDX, n_domLow, n_domHigh,
			n_father);
	for (int i = 0; i < 8; i++) {
		newnode->n_children[i] = n_children[i];
	}
	newnode->n_ptCount = n_ptCount;

	g_NodeList.erase(n_id);

	//g_NodeList.insert(make_pair(n_id,newnode));
	g_NodeList.insert(pair<int, OctTNode*>(n_id, newnode));
}

void OctLeafNode::rangeQueryNode(double *low, double *high,
		vector<OctPoint*> &pt_vector) {

	hash_set<IDTYPE>::iterator ite;
	if (calOverlap(low, high)) {
		for (ite = data.begin(); ite != data.end(); ite++) {
			OctPoint *tmp = g_PtList.find(*ite)->second;
			if (containPoint(tmp, low, high)) {
				pt_vector.push_back(tmp);
			}
		}
	}
}

