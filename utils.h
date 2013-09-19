/*
 * head.h
 *
 *  Created on: Sep 16, 2013
 *      Author: meibenjin
 */

#ifndef HEAD_H_
#define HEAD_H_

#define MAX_IP_ADDR_LENGTH 20

enum {
	SUCESS = 0, FAILED = -1
};

enum OP {
	HELLO = 80, READ, WRITE,
};



#endif /* HEAD_H_ */
