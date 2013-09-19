/*
 * utils.h
 *
 *  Created on: Sep 16, 2013
 *      Author: meibenjin
 */

#ifndef UTILS_H_
#define UTILS_H_

// limits for torus
#define MAX_NEIGHBORS 6
#define MAX_NODES_NUM 10 * 10 * 10


// 3-dimension coordinate
typedef struct coordinate
{
	int x;
	int y;
	int z;
}coordinate;

// return status of functions
enum status
{
	SUCESS = 0, FAILED = -1
};

//operations for torus nodes in communication 
enum OP
{
	HELLO = 80, READ, WRITE,
};

#endif /* UTILS_H_ */



