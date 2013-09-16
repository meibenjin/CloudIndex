/*
 * connect_server_test.c
 *
 *  Created on: Sep 13, 2013
 *      Author: meibenjin
 */

/*
 * socket_test.c
 *
 *  Created on: Sep 12, 2013
 *      Author: meibenjin
 */

#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include"../socket/socket_def.h"

#define COMMAND_LENGTH 10000

void show_bytes(char* start, int len){
    int i;
    for (i = 0; i < len; i++){
        printf(" %.2x", start[i]);
    }
    printf("\n");
}

int main(int argc, char **argv) {
	if (argc != 3) {
		printf("usage: ./%s serverip \n", argv[0]);
		exit(1);
	}

	int socketfd;
	struct sockaddr_in server_addr;
	socketfd = socket(AF_INET, SOCK_STREAM, 0);

	bzero(&server_addr, sizeof(struct sockaddr_in));
	server_addr.sin_family = AF_INET;
	if (inet_aton(argv[1], &server_addr.sin_addr) == 0) {
		printf("convert server ip failed!\n");
		exit(1);
	}
	server_addr.sin_port = htons(LISTEN_PORT);

	if (connect(socketfd, (struct sockaddr *) &server_addr,
			sizeof(struct sockaddr)) < 0) {
		printf("connect server failed!\n");
		exit(1);
	}

    //char command[COMMAND_LENGTH + 1];
    //memset(command, 0, sizeof(command));
    //strncpy(command, argv[2], COMMAND_LENGTH);


    message msg;
    msg.OP = HELLO;
    strncpy(msg.src, "192.168.11.134", MAX_IP_ADDR_LENGTH);
    strncpy(msg.dst, "192.168.11.133", MAX_IP_ADDR_LENGTH);

    //发送数据缓冲区
    //char buffer[SOCKET_BUF_SIZE];
    //memset(buffer, 0, sizeof(buffer));
    //memcpy(buffer, &msg, sizeof(message));

    printf("begin send message:\n");
    send(socketfd, (void *)&msg, sizeof(message), 0);

	return 0;
}


