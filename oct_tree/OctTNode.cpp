#include "OctTree.h"

/*	dim0
 //--------\
//| 4 | 5 |
 //--------- dim1               dim2 = 1
 //| 6 | 7 |
 //--------/

 //	dim0
 //--------\
//| 0 | 1 |
 //--------- dim1               dim 2 = 0
 //| 2 | 3 |
 //--------*/

OctTNode::OctTNode(int nid, NodeType type, double* low, double* high,
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

uint32_t OctTNode::getByteArraySize() {
	size_t data_size = data.size();
	return (sizeof(NodeType) + sizeof(int) * 11 + sizeof(double) * 3 * 2
			+ sizeof(size_t) + sizeof(int) * data_size);
}
void OctTNode::storeToByteArray(char **pdata, uint32_t &len) {
	len = getByteArraySize();
	*pdata = new char[len];
	char *ptr = *pdata;

	memcpy(ptr, &n_type, sizeof(NodeType));
	ptr += sizeof(NodeType);
	memcpy(ptr, &n_id, sizeof(int));
	ptr += sizeof(int);

	memcpy(ptr, &n_ptCount, sizeof(int));
	ptr += sizeof(int);
	memcpy(ptr, n_domLow, sizeof(double) * 3);
	ptr += sizeof(double) * 3;
	memcpy(ptr, n_domHigh, sizeof(double) * 3);
	ptr += sizeof(double) * 3;
	memcpy(ptr, &n_father, sizeof(int));
	ptr += sizeof(int);
	memcpy(ptr, n_children, sizeof(int) * 8);
	ptr += sizeof(int) * 8;

	size_t data_size = data.size();
	memcpy(ptr, &data_size, sizeof(size_t));
	ptr += sizeof(size_t);

	hash_set<IDTYPE>::iterator it;
	for (it = data.begin(); it != data.end(); it++) {
		memcpy(ptr, &(*it), sizeof(int));
		ptr += sizeof(int);
	}

}
void OctTNode::loadFromByteArray(const char *ptr) {
	memcpy(&n_type, ptr, sizeof(NodeType));
	ptr += sizeof(NodeType);
	memcpy(&n_id, ptr, sizeof(int));
	ptr += sizeof(int);
	memcpy(&n_ptCount, ptr, sizeof(int));
	ptr += sizeof(int);
	memcpy(n_domLow, ptr, sizeof(double) * 3);
	ptr += sizeof(double) * 3;
	memcpy(n_domHigh, ptr, sizeof(double) * 3);
	ptr += sizeof(double) * 3;
	memcpy(&n_father, ptr, sizeof(int));
	ptr += sizeof(int);
	memcpy(n_children, ptr, sizeof(int) * 8);
	ptr += sizeof(int) * 8;

	size_t data_size = 0;
	memcpy(&data_size, ptr, sizeof(size_t));
	ptr += sizeof(size_t);

	size_t i;
	for (i = 0; i < data_size; i++) {
		int p_id;
		memcpy(&p_id, ptr, sizeof(int));
		ptr += sizeof(int);
		data.insert(p_id);
	}
}
bool OctTNode::containPoint(OctPoint *pt) {
	for (int i = 0; i < 3; i++)
		if (pt->p_xyz[i] < n_domLow[i] - eps
				|| pt->p_xyz[i] > n_domHigh[i] + eps) {
			return false;
		}
	return true;
}
bool OctTNode::containPoint(OctPoint *pt, double *low, double *high) {
	for (int i = 0; i < 3; i++)
		if (pt->p_xyz[i] < low[i] - eps || pt->p_xyz[i] > high[i] + eps) {
			return false;
		}
	return true;
}
bool OctTNode::containPoint(OctPoint *pt, int idx) {
	double slow[3];
	double shigh[3];
	octsplit(slow, shigh, idx, n_domLow, n_domHigh);

	for (int i = 0; i < 3; i++)
		if (pt->p_xyz[i] < slow[i] - eps || pt->p_xyz[i] > shigh[i] + eps) {
			return false;
		}
	return true;
}
bool OctTNode::containPoint(double* xyz, int idx) {
	double slow[3];
	double shigh[3];
	octsplit(slow, shigh, idx, n_domLow, n_domHigh);

	for (int i = 0; i < 3; i++)
		if (xyz[i] < slow[i] - eps || xyz[i] > shigh[i] + eps) {
			return false;
		}
	return true;
}
void OctTNode::octsplit(double* slow, double* shigh, char idx, double* low,
		double* high) {
	memcpy(slow, low, 3 * sizeof(double));
	memcpy(shigh, high, 3 * sizeof(double));

	double mid[3];
	mid[0] = (high[0] + low[0]) / 2.0;
	mid[1] = (high[1] + low[1]) / 2.0;
	mid[2] = (high[2] + low[2]) / 2.0;

	// for the 3rd dim
	if (4 <= idx && 8 > idx) {
		slow[2] = mid[2];
	} else {
		shigh[2] = mid[2];
	}

	// for the remaining 2 dims
	int flag_idx = idx % 4;
	switch (flag_idx) {
	case 0:
		shigh[0] = mid[0];
		shigh[1] = mid[1];
		break;
	case 1:
		slow[0] = mid[0];
		shigh[1] = mid[1];
		break;
	case 2:
		shigh[0] = mid[0];
		slow[1] = mid[1];
		break;
	case 3:
		slow[0] = mid[0];
		slow[1] = mid[1];
		break;
	}
}

bool OctTNode::calOverlap(const double *low, const double *high) {
	double slow[3];
	double shigh[3];
	for (int i = 0; i < 3; i++) {
		slow[i] = low[i] < n_domLow[i] ? n_domLow[i] : low[i];
		shigh[i] = high[i] > n_domHigh[i] ? n_domHigh[i] : high[i];
		if (slow[i] >= shigh[i]) {
			return false;
		}
	}
	return true;
}
