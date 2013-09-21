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
	if (!node_ptr)
    {
		printf("set_node_ip: node_ptr is null pointer\n");
		return;
	}
	strncpy(node_ptr->info.ip, ip, MAX_IP_ADDR_LENGTH);
}

void get_node_ip(torus_node node, char *ip)
{
    if( ip == NULL)
    {
        printf("get_node_ip: ip is null pointer\n");
        return;
    }
    strncpy(node.info.ip, ip, MAX_IP_ADDR_LENGTH);
}

void set_coordinate(torus_node *node_ptr, int x, int y, int z)
{
	if (!node_ptr)
    {
		printf("set_coordinate: node_ptr is null pointer\n");
		return;
	}
	node_ptr->info.id.x = x;
	node_ptr->info.id.y = y;
	node_ptr->info.id.z = z;
}

void set_neighbors_num(torus_node *node_ptr, int neighbors_num)
{
	if (!node_ptr)
    {
		printf("set_neighbors_num: node_ptr is null pointer\n");
		return;
	}

    node_ptr->info.neighbors_num = neighbors_num;
}

int get_neighbors_num(torus_node node)
{
    return node.info.neighbors_num;
}

void print_coordinate(torus_node node)
{
	printf("(%d, %d, %d)\n", node.info.id.x, node.info.id.y, node.info.id.z);
}

void init_torus_node(torus_node *node_ptr)
{
	int i;
	set_node_ip(node_ptr, "0.0.0.0");

	set_coordinate(node_ptr, -1, -1, -1);

    set_neighbors_num(node_ptr, 0);

	for (i = 0; i < MAX_NEIGHBORS; ++i)
    {
		node_ptr->neighbors[i] = NULL;
	}
}



