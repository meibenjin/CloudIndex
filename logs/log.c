/*
 * log.c
 *
 *  Created on: Oct 8, 2013
 *      Author: meibenjin
 */

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>
#include<stdarg.h>

#include"log.h"

//__asm__(".symver memcpy,memcpy@GLIBC_2.2.5");


void get_local_time(char* buffer) {
	time_t rawtime;
	struct tm* timeinfo;

	time(&rawtime);
	timeinfo = localtime(&rawtime);
	sprintf(buffer, "[%04d-%02d-%02d %02d:%02d:%02d]\t", (timeinfo->tm_year + 1900),
			timeinfo->tm_mon, timeinfo->tm_mday, timeinfo->tm_hour,
			timeinfo->tm_min, timeinfo->tm_sec);
}

void write_log(const char *file_name, const char *format, ...) {
	FILE *fp;
	fp = fopen(file_name, "ab+");
	if (fp == NULL) {
		printf("log: open file %s error.\n", file_name);
		return;
	}

    // write time stamp
	/*char now[32];
	memset(now, 0, sizeof(now));
	get_local_time(now);
	fwrite(now, strlen(now), 1, fp);*/

    char arg_buf[4096];

    va_list argptr;
    va_start(argptr, format);
    vsnprintf(arg_buf, 4096, format, argptr);
    va_end(argptr);

	fwrite(arg_buf, strlen(arg_buf), 1, fp);
	fclose(fp);
	fp = NULL;
}

