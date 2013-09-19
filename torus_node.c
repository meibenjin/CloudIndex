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

int set_partitions(int p_x, int p_y, int p_z)
{
	if((p_x < 2) || (p_y < 2) || (p_z < 2))
    {
		printf("the number of partitions too small!\n");
		return FAILED;
	}
	if((p_x > 10) || (p_y > 10) || (p_z > 10))
    {
		printf("the number of partitions too large!\n");
		return FAILED;
	}

	torus_p.p_x = p_x;
	torus_p.p_y = p_y;
	torus_p.p_z = p_z;

	return SUCESS;
}

int assign_node_ip(torus_node *node_ptr)
{
	if(!node_ptr)
    {
		printf("assign_node_ip: node_ptr is null pointer\n");
		return FAILED;
	}
	strncpy(node_ptr->node_ip, "0.0.0.0", MAX_IP_ADDR_LENGTH);
	return SUCESS;
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

int get_node_index(int x, int y, int z)
{
	return x * torus_p.p_y * torus_p.p_z + y * torus_p.p_z + z;
}

int set_neighbors(torus_node *node_ptr, int x, int y, int z)
{
	if (!node_ptr)
    {
		printf("set_neighbors: node_ptr is null pointer\n");
		return FAILED;
	}

	// x-coordinate of the right neighbor of node_ptr on x-axis
	int xrc = (x + torus_p.p_x + 1) % torus_p.p_x;
	// x-coordinate of the left neighbor of node_ptr on x-axis
	int xlc = (x + torus_p.p_x - 1) % torus_p.p_x;

	// y-coordinate of the right neighbor of node_ptr on y-axis
	int yrc = (y + torus_p.p_y + 1) % torus_p.p_y;
	// y-coordinate of the left neighbor of node_ptr on y-axis
	int ylc = (y + torus_p.p_y - 1) % torus_p.p_y;

	// z-coordinate of the right neighbor of node_ptr on z-axis
	int zrc = (z + torus_p.p_z + 1) % torus_p.p_z;
	// z-coordinate of the left neighbor of node_ptr on z-axis
	int zlc = (z + torus_p.p_z - 1) % torus_p.p_z;

	// index of the right neighbor of node_ptr on x-axis
	int xri = get_node_index(xrc, y, z);
	// index of the left neighbor of node_ptr on x-axis
	int xli = get_node_index(xlc, y, z);

	// index of the right neighbor of node_ptr on y-axis
	int yri = get_node_index(x, yrc, z);
	// index of the left neighbor of node_ptr on y-axis
	int yli = get_node_index(x, ylc, z);

	// index of the right neighbor of node_ptr on z-axis
	int zri = get_node_index(x, y, zrc);
	// index of the left neighbor of node_ptr on z-axis
	int zli = get_node_index(x, y, zlc);

    int i = 0;
	node_ptr->neighbors[i++] = &torus_node_list[xri];
	if (xri != xli)
    {
		node_ptr->neighbors[i++] = &torus_node_list[xli];
	}
	node_ptr->neighbors[i++] = &torus_node_list[yri];
	if (yri != yli)
    {
		node_ptr->neighbors[i++] = &torus_node_list[yli];
	}
	node_ptr->neighbors[i++] = &torus_node_list[zri];
	if (zri != zli)
    {
		node_ptr->neighbors[i++] = &torus_node_list[zli];
	}

    node_ptr->neighbors_num = i;

	return SUCESS;
}

void print_neighbors(torus_node node)
{
	int i;
	for (i = 0; i < node.neighbors_num; ++i)
    {
		if (node.neighbors[i])
        {
			printf("\t");
			print_coordinate(*node.neighbors[i]);
		}
	}
}

int init_torus_node(torus_node *node_ptr)
{
	int i;

	if (assign_node_ip(node_ptr) == FAILED)
    {
		return FAILED;
	}

	set_coordinate(node_ptr, -1, -1, -1);
    node_ptr->neighbors_num = 0;

	for (i = 0; i < MAX_NEIGHBORS; ++i)
    {
		node_ptr->neighbors[i] = NULL;
	}

	return SUCESS;
}

int create_torus() 
{
	int i, j, k, index = 0;
	torus_node_num = torus_p.p_x * torus_p.p_y * torus_p.p_z;

	torus_node_list = (torus_node *) malloc(
			sizeof(torus_node) * torus_node_num);
	if (!torus_node_list)
    {
		printf("malloc torus node list failed!\n");
		return FAILED;
	}

	for (i = 0; i < torus_p.p_x; ++i)
    {
		for (j = 0; j < torus_p.p_y; ++j)
        {
			for (k = 0; k < torus_p.p_z; ++k)
            {
				torus_node *new_node = (torus_node *) malloc(
						sizeof(torus_node));
				new_node = &torus_node_list[index];
				if (init_torus_node(new_node) == FAILED)
                {
					printf("initial torus node failed!\n");
					return FAILED;
				}
				set_coordinate(new_node, i, j, k);
				index++;
			}
		}
	}

	index = 0;
	for (i = 0; i < torus_p.p_x; ++i)
    {
		for (j = 0; j < torus_p.p_y; ++j)
        {
			for (k = 0; k < torus_p.p_z; ++k)
            {
				set_neighbors(&torus_node_list[index], i, j, k);
				index++;
			}
		}
	}

	return SUCESS;
}

void print_torus()
{
	int i;
	for (i = 0; i < torus_node_num; ++i)
    {
		print_coordinate(torus_node_list[i]);
		print_neighbors(torus_node_list[i]);
	}
}

int dispatch_torus_nodes()
{
    int i, j, index;
    if(torus_node_list == NULL)
    {
        printf("dispatch_torus_node: torus_node_list is null\n");
        return FAILED;
    }

    for(i = 0; i < torus_node_num; ++i)
    { 
        struct torus_node *node_ptr = NULL;
        node_ptr = &torus_node_list[i];

        int neighbors_num = node_ptr->neighbors_num;
        struct node_info neighbors[neighbors_num];

        for(j = 0; j < neighbors_num; ++j)
        {
            if(node_ptr && node_ptr->neighbors[j])
            {
                strncpy(neighbors[j].ip, node_ptr->neighbors[j]->node_ip, MAX_IP_ADDR_LENGTH);
                neighbors[j].id = node_ptr->neighbors[j]->node_id;
                index++;
            }
        }
        char *dst_ip = node_ptr->node_ip;
        // TODO send to dst_ip
    }
}

int main(int argc, char **argv)
{
	if(argc < 4)
    {
		printf("usage: %s x y z\n", argv[0]);
		exit(1);
	}

    // set 3-dimension partitions
	if(set_partitions(atoi(argv[1]), atoi(argv[2]), atoi(argv[3])) == FAILED)
    {
		exit(1);
	}

    // create a new torus
	if(create_torus() == FAILED)
    {
        // TODO free torus when create failed
        // free_torus
    }

	print_torus();

    dispatch_torus_nodes();

	return 0;
}

