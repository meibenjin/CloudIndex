/*
 * command.h
 *
 *  Created on: Jan 20, 2015
 *      Author: meibenjin
 */

#ifndef COMMAND_H_
#define COMMAND_H_

#include<stddef.h>
#include<pthread.h>
#include "utils.h"

// exec function of command
typedef int (*exec_func)(void *);

#pragma pack(push)
#pragma pack(4)

typedef struct command {
	char name[MAX_CMD_LEN];
	char doc[MAX_DOC_LEN];
    //used for cache the result of command
    char cache_file[MAX_FILE_NAME];
    int cmd_type;
	exec_func exec;
} command;

typedef struct command_mgr {
	command cmd_list[MAX_CMD_NUM];
	size_t cmd_num;

    command last_cmd;
	int running;
}command_mgr, *command_mgr_t;

#pragma pack(pop)

//command_list new_command_list();

int add_command(const char *name, const char *doc, int cmd_type, exec_func func);

command* find_command(const char *name);

int remove_command(const char *name);

void print_command_list();

// create a new connection manager instance
command_mgr* new_command_mgr();

pthread_t run_command_mgr();

void stop_command_mgr();

extern command_mgr_t the_cmd_mgr;

#endif /* COMMAND_H_ */
