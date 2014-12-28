
/*
 * log.h
 *
 *  Created on: Oct 8, 2013
 *      Author: meibenjin
 */

#ifndef LOG_H_
#define LOG_H_

void get_local_time(char* buffer);

void write_log(const char *file_name, const char *format, ...);

#endif /* LOG_H_ */
