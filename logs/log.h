/*
 * log.h
 *
 *  Created on: Oct 8, 2013
 *      Author: meibenjin
 */

#ifndef LOG_H_
#define LOG_H_

#define CTRL_NODE_LOG "../logs/control_node.log"
#define TORUS_NODE_LOG "../logs/torus_node.log"

char log_buf[1024];

void write_log(const char *file_name, const char *buffer);

#endif /* LOG_H_ */
