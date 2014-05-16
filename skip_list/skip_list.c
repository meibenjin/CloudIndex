/*
 * skip_list.c
 *
 *  Created on: Oct 2, 2013
 *      Author: meibenjin
 */

#include<stdlib.h>
#include<stdio.h>
#include<string.h>

#include"skip_list.h"
#include"logs/log.h"
#include"torus_node/torus_node.h"

//__asm__(".symver memcpy,memcpy@GLIBC_2.2.5");

// check two MAX_DIM_NUM dimension regions
// note that this function remain correct when 
// one of the two region is a point(low = high) 
int overlaps(interval c[], interval o[]) {
	int i, ovlp = 1;
	i = 0;
	while (ovlp && i < MAX_DIM_NUM) {
		ovlp = !(c[i].high < o[i].low || c[i].low > o[i].high);
		i++;
	}

	return ovlp;
}

int point_contain(point p, interval o[]) {
	int i, ovlp = 1;
	i = 0;
	while (ovlp && i < MAX_DIM_NUM) {
		ovlp = !(p.axis[i] < o[i].low || p.axis[i] > o[i].high);
		i++;
	}
    return ovlp;
}

int line_intersect(interval o[], point start, point end) {
    int intersect = 1;
    int i = 0;
    interval c[MAX_DIM_NUM];
    for(i = 0; i < MAX_DIM_NUM; i++) {
        c[i].low = start.axis[i] < end.axis[i] ? start.axis[i] : end.axis[i];
        c[i].high = start.axis[i] > end.axis[i] ? start.axis[i] : end.axis[i];
    }
    intersect = overlaps(c, o);

    return intersect;
}

int line_contain(interval o[], point start, point end) {
    // region o contain one point at least return true
    if((1 == point_contain(start, o)) && (1 == point_contain(end, o))) {
        return 1;
    }
    return 0;
}

data_type get_distance(interval c, interval o){
	data_type c_center = (c.low + c.high) / 2;
	data_type o_center = (o.low + o.high) / 2;
	data_type dis = c_center - o_center;
	return (dis > 0) ? dis : (-1 * dis);
}

int compare(interval cinterval, interval ointerval) {
	if (cinterval.low <= ointerval.low) {
		return -1;
	} else {
		return 1;
	}
}

// TODO maybe optimize future
int random_level() {
	int level = 0, randint;
	randint = rand();
	for (; (randint % 2) == 0; randint /= 2) {
		level++;
	}

	if (level > MAXLEVEL) {
		level = MAXLEVEL;
	}

	return (level < MAXLEVEL) ? level : MAXLEVEL;
}

skip_list_node *new_skip_list_node(int level, node_info *leaders) {
	skip_list_node *sln = (skip_list_node *) malloc(
			sizeof(*sln) + (level + 1) * sizeof(struct skip_list_level));
	if (sln == NULL) {
		printf("new_skip_list_node:insufficient memory.\n");
		return NULL;
	}

	int i;
	// if leaders is NULL pointer
    // set leader as inital torus node info
    node_info node;
    init_node_info(&node);
    for(i = 0; i < LEADER_NUM; ++i) {
        if (leaders == NULL) {
            sln->leader[i] = node;
        } else {
            sln->leader[i] = leaders[i];
        }
    }
	sln->height = level;

	for (i = 0; i <= level; ++i) {
		sln->level[i].forward = NULL;
		sln->level[i].backward = NULL;
	}
	return sln;
}

skip_list *new_skip_list(int level) {
	skip_list *slist;
	slist = (skip_list *) malloc(sizeof(*slist));
	if (slist == NULL) {
		printf("init_list:insufficient memory.\n");
		return NULL;
	}
	slist->level = 0;
	slist->header = new_skip_list_node(level, NULL);
	if (NULL == slist->header) {
		return NULL;
	}
	return slist;
}

int insert_skip_list(skip_list *slist, node_info *node_ptr) {
	/*if (TRUE == search_skip_list(slist, node_ptr)) {
	 printf("insert: duplicate torus node.\n");
	 return FALSE;
	 } else {*/
	/*int i, new_level;
	skip_list_node *sln_ptr;
	skip_list_node *new_sln;

	// random choose a level
	new_level = random_level();

	new_sln = new_skip_list_node(new_level, node_ptr);
	if (new_sln == NULL) {
		return FALSE;
	}

	if (new_level > slist->level) {
		slist->level = new_level;
	}

	sln_ptr = slist->header;
	for (i = slist->level; i >= 0; --i) {
		// region[2] is default the time dimension
		while ((sln_ptr->level[i].forward != NULL)
				&& (-1
						== compare(sln_ptr->level[i].forward->leader.region[2],
								node_ptr->region[2]))) {
			sln_ptr = sln_ptr->level[i].forward;
		}

		if (i <= new_level) {
			new_sln->level[i].backward =
					(sln_ptr == slist->header) ? NULL : sln_ptr;
			if (sln_ptr->level[i].forward) {
				new_sln->level[i].forward->level[i].backward = new_sln;
			}
			new_sln->level[i].forward = sln_ptr->level[i].forward;

			sln_ptr->level[i].forward = new_sln;
		}
	}*/
	return TRUE;

}

int remove_skip_list(skip_list *slist, node_info *node_ptr) {
	/*if (FALSE == search_skip_list(slist, node_ptr)) {
	 printf("remove: can't find torus node.\n");
	 return FALSE;
	 } else {*/
	/*int i;
	skip_list_node *sln_ptr;
	skip_list_node *del_ptr;
	sln_ptr = slist->header;

	for (i = slist->level; i >= 0; --i) {
		while ((sln_ptr->level[i].forward != NULL)
				&& (-1
						== compare(sln_ptr->level[i].forward->leader.region[2],
								node_ptr->region[2]))) {
			sln_ptr = sln_ptr->level[i].forward;
		}
		if (i == 0) {
			del_ptr = sln_ptr->level[0].forward;
		}
		if ((sln_ptr->level[i].forward) != NULL
				&& (0
						== compare(sln_ptr->level[i].forward->leader.region[2],
								node_ptr->region[2]))) {
			sln_ptr->level[i].forward =
					sln_ptr->level[i].forward->level[i].forward;
			if (sln_ptr->level[i].forward != NULL) {
				sln_ptr->level[i].forward->level[i].backward =
						(sln_ptr == slist->header) ? NULL : sln_ptr;
			}
		}
	}

	free(del_ptr);

	// adjust header level 
	while ((slist->level > 0)
			&& (slist->header->level[slist->level].forward == NULL)) {
		--slist->level;
	}*/
	return TRUE;
	//}
}

int interval_overlap(interval cinterval, interval ointerval) {

	if(cinterval.high < ointerval.low){
		return -1;
	}

	if (cinterval.low > ointerval.high) {
		return 1;
	}

	return 0;
}

node_info *search_skip_list(skip_list *slist, interval time_interval) {
	/*int i;
	skip_list_node *sln_ptr;
	sln_ptr = slist->header;

	// find the torus node position
	for (i = slist->level; i >= 0; --i) {
		while ((sln_ptr->level[i].forward != NULL)
				&& (0 != interval_overlap(sln_ptr->level[i].forward->leader.region[2], time_interval))) {
			sln_ptr = sln_ptr->level[i].forward;
		}
	}

	sln_ptr = sln_ptr->level[0].forward;
	if ((sln_ptr != NULL)
			&& (0 == interval_overlap(sln_ptr->leader.region[2], time_interval))) {
		return &sln_ptr->leader;
	}*/
	return NULL;
}

void print_skip_list(skip_list *slist) {
	int i, j, k;
	skip_list_node *sln_ptr;
	for (i = 0; i <= slist->level; ++i) {
		sln_ptr = slist->header;
		printf("level:%d\n", i);
		while (sln_ptr->level[i].forward != NULL) {
            for(j = 0; j < LEADER_NUM; j++) {
                printf("%s ", sln_ptr->level[i].forward->leader[j].ip);
                //print_node_info(sln_ptr->level[i].forward->leader[j]);
            }
            printf("height:%d\n", sln_ptr->level[i].forward->height);

            if (sln_ptr->level[i].forward->level[i].backward) {
                printf("\tback:");
                for(k = 0; k < LEADER_NUM; k++) {
                    printf("%s ", sln_ptr->level[i].forward->level[i].backward->leader[k].ip);
                    //print_node_info(sln_ptr->level[i].forward->level[j].backward->leader[k]);
                }
                printf("\n");
            } else {
                printf("\tback:NULL\n");
            }
            sln_ptr = sln_ptr->level[i].forward;
		}
		printf("\n");
	}
}

void print_skip_list_node(skip_list *slist) {
    #ifdef WRITE_LOG 
        int i = 0, j;
        char buf[1024];
        skip_list_node *sln_ptr;
        sln_ptr = slist->header;
        write_log(TORUS_NODE_LOG, "skip list:\n");
        for(j = 0; j < LEADER_NUM; j++) {
            print_node_info(sln_ptr->leader[j]);
        }

        while (i <= slist->level) {
            memset(buf, 0, 1024);
            sprintf(buf, "level:%d\n", i);
            write_log(TORUS_NODE_LOG, buf);

            memset(buf, 0, 1024);
            if (sln_ptr->level[i].forward != NULL) {
                write_log(TORUS_NODE_LOG, "\tforward: ");
                int len = 0;
                for(j = 0; j < LEADER_NUM; j++) {
                    printf("%s ", sln_ptr->level[i].forward->leader[j].ip);
                    len += sprintf(buf + len, "%s ", sln_ptr->level[i].forward->leader[j].ip);
                    //print_node_info(sln_ptr->level[i].forward->leader[j]);
                }
                printf("\n");
                sprintf(buf + len, "\n");
                write_log(TORUS_NODE_LOG, buf);
            } else {
                printf("\tforward: NULL\n");
                write_log(TORUS_NODE_LOG, "\tforward: NULL\n");
            }
            if (sln_ptr->level[i].backward != NULL) {
                write_log(TORUS_NODE_LOG, "\tbackward: ");
                int len = 0;
                for(j = 0; j < LEADER_NUM; j++) {
                    printf("%s ", sln_ptr->level[i].backward->leader[j].ip);
                    len += sprintf(buf + len, "%s ", sln_ptr->level[i].backward->leader[j].ip);
                    //print_node_info(sln_ptr->level[i].backward->leader[j]);
                }
                printf("\n");
                sprintf(buf + len, "\n");
                write_log(TORUS_NODE_LOG, buf);
            } else {
                printf("\tbackward: NULL\n");
                write_log(TORUS_NODE_LOG, "\tbackward: NULL\n");
            }
            i++;
        }
    #endif
}

