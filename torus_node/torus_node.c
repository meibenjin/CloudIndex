/*
 * torus_node.c
 *
 *  Created on: Sep 16, 2013
 *      Author: meibenjin
 */

#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include"torus_node.h"

void set_node_ip(torus_node *node_ptr, const char *ip)
{
	strncpy(node_ptr->node_ip, ip, MAX_IP_ADDR_LENGTH);
}

void set_coordinate(torus_node *node_ptr, int x, int y, int z)
{
	if (!node_ptr)
    {
		printf("set_coordinate: node_ptr is null pointer\n");
		return;
	}
	node_ptr->node_id.x = x;
	node_ptr->node_id.y = y;
	node_ptr->node_id.z = z;
}

void print_coordinate(torus_node node)
{
	printf("(%d, %d, %d)\n", node.node_id.x, node.node_id.y, node.node_id.z);
}

void init_torus_node(torus_node *node_ptr)
{
	int i;
	set_node_ip(node_ptr, "0.0.0.0");

	set_coordinate(node_ptr, -1, -1, -1);
    node_ptr->neighbors_num = 0;

	for (i = 0; i < MAX_NEIGHBORS; ++i)
    {
		node_ptr->neighbors[i] = NULL;
	}
}



