#include "OctTree.h"

extern "C" {
#include "utils.h"
#include "logs/log.h"
}

OctPoint::OctPoint(IDTYPE pid,double time,double *xyz, IDTYPE tid,IDTYPE pre,IDTYPE next){
	p_id = pid;
	p_time = time;
	p_xyz[0] = xyz[0];
	p_xyz[1] = xyz[1];
	p_xyz[2] = xyz[2];
	p_tid = tid;
	this->pre = pre;
	this->next = next;
}

void OctPoint::printIt() {
    FILE *fp;
    char buffer[1024];
    fp = fopen(RESULT_LOG, "ab+");
    if (fp == NULL) {
        printf("log: open file %s error.\n", RESULT_LOG);
        return;
    }
    sprintf(buffer, "id:%d pre:%d next:%d x:%lf y:%lf z:%lf traj:%d\n", \
            p_id, pre, next, p_xyz[0], p_xyz[1], p_xyz[2], p_tid);
    fwrite(buffer, strlen(buffer), 1, fp);
    fclose(fp);
}

OctPoint *OctPoint::clone(){
    OctPoint *point = new OctPoint(p_id, p_time, p_xyz, p_tid, this->pre, this->next);
    return point;
}


bool OctPoint::isNear(IDTYPE pid){
	//if(pid==p_id)return false;
	OctPoint *pt = g_PtList.find(pid)->second;
	double distance = 0;
	for(int i=0;i<3;i++){
		distance += (p_xyz[i] - pt->p_xyz[i]) * (p_xyz[i] - pt->p_xyz[i]);
	}
	if(sqrt(distance)<miniDistance)
		return true;
	else 
		return false;
}

uint32_t OctPoint::getByteArraySize() {
	return (sizeof(int) * 4 + sizeof(double) * 4 + sizeof(short) * 1);
}

void OctPoint::storeToByteArray(char **data, uint32_t &len) {
	len = getByteArraySize();
	*data = new char[len];
	char *ptr = *data;

	memcpy(ptr, &p_id, sizeof(int));
	ptr += sizeof(int);
	memcpy(ptr, &p_time, sizeof(double));
	ptr += sizeof(double);
	memcpy(ptr, &p_level, sizeof(short));
	ptr += sizeof(short);
	memcpy(ptr, p_xyz, sizeof(double) * 3);
	ptr += sizeof(double) * 3;
	memcpy(ptr, &p_tid, sizeof(int));
	ptr += sizeof(int);
	memcpy(ptr, &pre, sizeof(int));
	ptr += sizeof(int);
	memcpy(ptr, &next, sizeof(int));
	//ptr += sizeof(int);

}

void OctPoint::loadFromByteArray(const char *ptr) {

	memcpy(&p_id, ptr, sizeof(int));
	ptr += sizeof(int);
	memcpy(&p_time, ptr, sizeof(double));
	ptr += sizeof(double);
	memcpy(&p_level, ptr, sizeof(short));
	ptr += sizeof(short);
	memcpy(p_xyz, ptr, sizeof(double) * 3);
	ptr += sizeof(double) * 3;
	memcpy(&p_tid, ptr, sizeof(int));
	ptr += sizeof(int);
	memcpy(&pre, ptr, sizeof(int));
	ptr += sizeof(int);
	memcpy(&next, ptr, sizeof(int));
	//ptr += sizeof(int);
}



Traj::Traj(IDTYPE id,IDTYPE head_id,IDTYPE tail_id){
	t_id = id;
	t_tail = tail_id;
	t_head = head_id;
}

void Traj::printIt() {
    FILE *fp;
    char buffer[1024];
    fp = fopen(RESULT_LOG, "ab+");
    if (fp == NULL) {
        printf("log: open file %s error.\n", RESULT_LOG);
        return;
    }
    sprintf(buffer, "tid:%d head:%d tail:%d\n", t_id, t_head, t_tail);
    fwrite(buffer, strlen(buffer), 1, fp);
    fclose(fp);
}

int Traj::printTraj() {
    OctPoint *cur_point;
    IDTYPE pid;
    IDTYPE head_id = this->t_head;
    IDTYPE tail_id = this->t_tail;
    char buffer[DATA_SIZE];
    size_t cpy_len = 0;

    pid = head_id;
    do {
        cur_point = g_PtList.find(pid)->second;
        if(cpy_len + 100 < DATA_SIZE) {
            cpy_len += sprintf(buffer + cpy_len, "%d,%d,[%.4lf %.4lf %.4lf] ", \
                    cur_point->p_id, cur_point->p_tid,\
                    cur_point->p_xyz[0], cur_point->p_xyz[1], cur_point->p_xyz[2]);
        }
        pid = cur_point->next;
        if(pid == -1) {
            write_log(ERROR_LOG, "printTraj:%d traj occurred error", this->t_id);
            return FALSE;
        }
    }while(cur_point->next != tail_id);

    cpy_len += sprintf(buffer + cpy_len, "\n");
    write_log(OCT_TREE_LOG, buffer);
    return TRUE;
}

uint32_t Traj::getByteArraySize() {
	return (sizeof(IDTYPE) * 3);
}

void Traj::storeToByteArray(char **data, uint32_t &len) {
	len = getByteArraySize();
	*data = new char[len];
	char *ptr = *data;

	memcpy(ptr, &t_id, sizeof(IDTYPE));
	ptr += sizeof(IDTYPE);
	memcpy(ptr, &t_head, sizeof(IDTYPE));
	ptr += sizeof(IDTYPE);
	memcpy(ptr, &t_tail, sizeof(IDTYPE));
	//ptr += sizeof(IDTYPE);
}

void Traj::loadFromByteArray(const char *ptr) {
	memcpy(&t_id, ptr, sizeof(IDTYPE));
	ptr += sizeof(IDTYPE);
	memcpy(&t_head, ptr, sizeof(IDTYPE));
	ptr += sizeof(IDTYPE);
	memcpy(&t_tail, ptr, sizeof(IDTYPE));
	//ptr += sizeof(int);
}
