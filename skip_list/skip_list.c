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

//__asm__(".symver memcpy,memcpy@GLIBC_2.2.5");

int compare(node_info *node1, node_info *node2) {
	if (node1 == NULL) {
		return -1;
	}
	int id1 = node1->cluster_id;
	int id2 = node2->cluster_id;
	if (id1 < id2) {
		return -1;
	} else if (id1 == id2) {
		return 0;
	} else {
		return 1;
	}
}

// TODO maybe optimize future
int random_level() {
	int level = 0, randint;
	randint = rand();
	for (; (randint % 3) == 0; randint /= 3) {
		level++;
	}

	if (level > MAXLEVEL) {
		level = MAXLEVEL;
	}
	return level;
}

skip_list_node *new_skip_list_node(int level, node_info *node_ptr) {
	skip_list_node *sln = (skip_list_node *) malloc(
			sizeof(*sln) + (level + 1) * sizeof(struct skip_list_level));
	if (sln == NULL) {
		printf("new_skip_list_node:insufficient memory.\n");
		return NULL;
	}
	sln->leader = node_ptr;
    sln->height = level;

	int i;
	for (i = 0; i <= level; ++i) {
		sln->level[i].forward = NULL;
		sln->level[i].backward = NULL;
	}
	return sln;
}

skip_list *new_skip_list() {
	skip_list *slist;
	slist = (skip_list *) malloc(sizeof(*slist));
	if (slist == NULL) {
		printf("init_list:insufficient memory.\n");
		return NULL;
	}
	slist->level = 0;
	slist->header = new_skip_list_node(MAXLEVEL, NULL);
	if (NULL == slist->header) {
		return NULL;
	}
	return slist;
}

int insert_skip_list(skip_list *slist, node_info *node_ptr) {
	if (TRUE == search_skip_list(slist, node_ptr)) {
		printf("insert: duplicate torus node.\n");
		return FALSE;
	} else {
		int i, new_level;
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
			while ((sln_ptr->level[i].forward != NULL)
					&& (-1
							== compare(sln_ptr->level[i].forward->leader,
									node_ptr))) {
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
		}
		return TRUE;
	}

}

int remove_skip_list(skip_list *slist, node_info *node_ptr) {
	if (FALSE == search_skip_list(slist, node_ptr)) {
		printf("remove: can't find torus node.\n");
		return FALSE;
	} else {
		int i;
		skip_list_node *sln_ptr;
		skip_list_node *del_ptr;
		sln_ptr = slist->header;

		for (i = slist->level; i >= 0; --i) {
			while ((sln_ptr->level[i].forward != NULL)
					&& (-1
							== compare(sln_ptr->level[i].forward->leader,
									node_ptr))) {
				sln_ptr = sln_ptr->level[i].forward;
			}
			if (i == 0) {
				del_ptr = sln_ptr->level[0].forward;
			}
			if ((sln_ptr->level[i].forward) != NULL
					&& (0
							== compare(sln_ptr->level[i].forward->leader,
									node_ptr))) {
				sln_ptr->level[i].forward =
						sln_ptr->level[i].forward->level[i].forward;
				if (sln_ptr->level[i].forward != NULL) {
					sln_ptr->level[i].forward->level[i].backward =
							(sln_ptr == slist->header) ? NULL : sln_ptr;
				}
			}
		}

		free(del_ptr);

		/* adjust header level */
		while ((slist->level > 0)
				&& (slist->header->level[slist->level].forward == NULL)) {
			--slist->level;
		}
		return TRUE;
	}
}

int search_skip_list(skip_list *slist, node_info *node_ptr) {
	int i;
	skip_list_node *sln_ptr;
	sln_ptr = slist->header;

	// find the torus node position
	for (i = slist->level; i >= 0; --i) {
		while ((sln_ptr->level[i].forward != NULL)
				&& (-1 == compare(sln_ptr->level[i].forward->leader, node_ptr))) {
			sln_ptr = sln_ptr->level[i].forward;
		}
	}

	sln_ptr = sln_ptr->level[0].forward;
	if ((sln_ptr != NULL) && (0 == compare(sln_ptr->leader, node_ptr))) {
		return TRUE;
	}
	return FALSE;
}

void traverse_skip_list(skip_list *slist) {
	int i;
	skip_list_node *sln_ptr;
	for (i = 0; i <= slist->level; ++i) {
		sln_ptr = slist->header;
		printf("level:%d\n", i);
		while (sln_ptr->level[i].forward != NULL) {
			print_node_info(*sln_ptr->level[i].forward->leader);
			if (sln_ptr->level[i].forward->level[i].backward) {
				printf("\tback:");
				print_node_info(
						*sln_ptr->level[i].forward->level[i].backward->leader);
			} else {
				printf("\tback:NULL\n");
			}
			sln_ptr = sln_ptr->level[i].forward;
		}
		printf("\n");
	}
}

void print_skip_list_node(skip_list_node *sln_ptr) {
	/*char buf[1024];
	memset(buf, 0, 1024);
	write_log(TORUS_NODE_LOG, "skip list:\n");

	int i;
	while(i <= sln_ptr->level){
        memset(buf, 0, 1024);
        sprintf(buf, "level:%s\n", i);
        write_log(TORUS_NODE_LOG, buf);
        if(sln_ptr[i].has_forward == 1) {
            memset(buf, 0, 1024);
            write_log(TORUS_NODE_LOG,"forward: ");
            print_node_info(sln_ptr[i].forward);
        }
        if(sln_ptr[i].has_backward == 1) {
            memset(buf, 0, 1024);
            write_log(TORUS_NODE_LOG,"backward: ");
            print_node_info(sln_ptr->forward);
        }
		i++;
	}*/
    ;
}

skip_list_node *construct_skip_list_node(int nodes_num, node_info *node_ptr){
	skip_list_node *sln_ptr;
    int level = nodes_num - 1;
    sln_ptr = (skip_list_node *) malloc(sizeof(skip_list_node) * level);
    char buf[1024];
	if (sln_ptr == NULL) {
		memset(buf, 0, 1024);
		sprintf(buf, "sln_ptr is null");
		write_log(TORUS_NODE_LOG, buf);
		return NULL;
	}

    /*int i = 0;
    while(i <= level) {
        sln_ptr[i] = node_ptr[i];
        memset(buf, 0, 1024);
        sprintf(buf, "level:%d\n", i);
        write_log(TORUS_NODE_LOG, buf);
        if(sln_ptr[i].has_forward == 1) {
            memset(buf, 0, 1024);
            write_log(TORUS_NODE_LOG,"forward: ");
            print_node_info(sln_ptr[i].forward);
        }
        if(sln_ptr[i].has_backward == 1) {
            memset(buf, 0, 1024);
            write_log(TORUS_NODE_LOG,"backward: ");
            print_node_info(sln_ptr[i].backward);
        }
        i++;
    } */

	return sln_ptr;
}


