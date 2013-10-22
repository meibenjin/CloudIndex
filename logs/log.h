/*
 * log.h
 *
 *  Created on: Oct 8, 2013
 *      Author: meibenjin
 */

#ifndef LOG_H_
#define LOG_H_


char log_buf[1024];

void write_log(const char *file_name, const char *buffer);

#endif /* LOG_H_ */
