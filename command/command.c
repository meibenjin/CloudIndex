/*
 * command.c
 *
 *  Created on: Jan 20, 2015
 *      Author: meibenjin
 */

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<assert.h>

#include"command.h"
#include"logs/log.h"

#include"readline/readline.h"
#include"readline/history.h"

command_mgr_t the_cmd_mgr = NULL;

int add_command(const char *name, const char *doc, int cmd_type, exec_func func) {
    assert(the_cmd_mgr != NULL);

    int i = the_cmd_mgr->cmd_num;
    if(i == MAX_CMD_NUM) {
        write_log(ERROR_LOG, "add_command: commands num exceed %d.\n", MAX_CMD_NUM);
        return FALSE;
    }
    command *cmd = &the_cmd_mgr->cmd_list[i];
	strncpy(cmd->name, name, MAX_CMD_LEN);
	strncpy(cmd->doc, doc, MAX_DOC_LEN);
    cmd->cmd_type = cmd_type;
    snprintf(cmd->cache_file, MAX_FILE_NAME, "%s/cache_%s", LOG_DIR, cmd->name);
	cmd->exec = func;
	the_cmd_mgr->cmd_num++;
	return TRUE;
}

command* find_command(const char *name) {
    assert(the_cmd_mgr != NULL);

    int i, num = the_cmd_mgr->cmd_num;
    command *cmd = NULL;
    for(i = 0; i < num; i++) {
        cmd = &the_cmd_mgr->cmd_list[i];
        if(strcasecmp(cmd->name, name) == 0) {
            return cmd; 
        }
    }
	return NULL;
}

int remove_command(const char *name) {

    assert(the_cmd_mgr != NULL);

    int i, j, num = the_cmd_mgr->cmd_num;
    command *cmd = NULL;
    for(i = 0; i < num; i++) {
        cmd = &the_cmd_mgr->cmd_list[i];
        if(strcasecmp(cmd->name, name) == 0) {
            break;
        }
    }

    // command not found
    if(i == num) {
        return FALSE;
    }

    for(j = i + 1; j < num; j++) {
        the_cmd_mgr->cmd_list[j-1] = the_cmd_mgr->cmd_list[j];
    } 
    the_cmd_mgr->cmd_num--;
    return TRUE;
}

void print_command_list() {
    assert(the_cmd_mgr != NULL);

    int i, num = the_cmd_mgr->cmd_num;
    command *cmd = NULL;
    for(i = 0; i < num; i++) {
        cmd = &the_cmd_mgr->cmd_list[i];
        printf("%-15s\t%10s\n", cmd->name, cmd->doc);
	}
    printf("\n");
}

static void init_command(command *cmd) {
    strcpy(cmd->name, "");
    strcpy(cmd->doc, "");
    sprintf(cmd->cache_file, "%s/null", LOG_DIR);
    cmd->cmd_type = LOCAL_CMD;
    cmd->exec = NULL;
}

command_mgr* new_command_mgr() {
	command_mgr* new_cmd_mgr;
	new_cmd_mgr = (command_mgr *) malloc(sizeof(struct command_mgr));
	if (new_cmd_mgr == NULL) {
		write_log(ERROR_LOG,
				"new_command_mgr: malloc for command manager failed.\n");
		return NULL;
	}

	new_cmd_mgr->cmd_num = 0;
    new_cmd_mgr->running = 0;

	return new_cmd_mgr;
}

char *read_cmd(char *prompt) {
    static char *line_read = NULL;
    if(line_read) {
        free(line_read);
        line_read = NULL;
    }

    line_read = readline(prompt);

    if(line_read && *line_read) {
        add_history(line_read);
    }
    return(line_read);
}

void* process_command(void *args) {
    printf("Welcome to EBST!\n\n");

    // init the last_cmd 
    init_command(&the_cmd_mgr->last_cmd);

    char *cmd_str = NULL;
	while (the_cmd_mgr->running) {

        cmd_str = read_cmd("EBST> ");
        if(strlen(cmd_str) == 0) {
            continue;
        }

		command *cmd = find_command(cmd_str);
        if(NULL != cmd) {
            if(REMOTE_CMD == cmd->cmd_type) {
                the_cmd_mgr->last_cmd = *cmd;
            }
            cmd->exec(NULL);
        } else {
            printf("command not found.\n");
        }
	}

    pthread_cancel(pthread_self());

    return NULL;
}


pthread_t run_command_mgr() {
    assert(the_cmd_mgr != NULL);
	the_cmd_mgr->running = 1;

	pthread_t cmd_thread;
	pthread_create(&cmd_thread, NULL, process_command, NULL);
	return cmd_thread;
}

void stop_command_mgr() {
    assert(the_cmd_mgr != NULL);
	the_cmd_mgr->running = 0;
	//TODO free space in command manager
}

