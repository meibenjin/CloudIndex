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

#include"log.h"

void get_local_time(char* buffer) {
	time_t rawtime;
	struct tm* timeinfo;

	time(&rawtime);
	timeinfo = localtime(&rawtime);
	sprintf(buffer, "[%04d-%02d-%02d %02d:%02d:%02d]\t", (timeinfo->tm_year + 1900),
			timeinfo->tm_mon, timeinfo->tm_mday, timeinfo->tm_hour,
			timeinfo->tm_min, timeinfo->tm_sec);
}

void write_log(const char *file_name, const char *buffer) {
	FILE *fp;
	fp = fopen(file_name, "ab+");
	if (fp == NULL) {
		printf("log: open file %s error.\n", file_name);
		return;
	}
	char now[32];
	memset(now, 0, sizeof(now));
	get_local_time(now);
	fwrite(now, strlen(now), 1, fp);
	fwrite(buffer, strlen(buffer), 1, fp);
	fclose(fp);
	fp = NULL;
}
