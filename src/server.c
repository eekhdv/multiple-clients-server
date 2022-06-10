/* Copyright 2022 Khadiev Edem
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <unistd.h>
#include <pthread.h>

#include "server.h"
#include "utils.h"

#define PORT "8765"
#define BUFFER_SIZE 65536
#define MAX_CLIENTS 100

pthread_t thread_clients[MAX_CLIENTS];
USER_INFO users[MAX_CLIENTS];

void *client_connection(void *vargp) {
	USER_INFO user = *(USER_INFO *)vargp;
	char buffer[BUFFER_SIZE] = {'\0'};
	while (1) {
		memset(buffer, 0, BUFFER_SIZE);
		recv(user.sockfd, buffer, BUFFER_SIZE, 0);
		if (strcmp(buffer, "exit\n") == 0) {
			client_close(user.sockfd);
			break;
		} 
		sendtoroom(buffer, user.room_number);
		printf("%s", buffer);
	}
	pthread_exit(NULL);
}

int init_server() {
	int status, sockfd, yes = 1; 
	struct addrinfo *servinfo;
	struct addrinfo hints = {
		.ai_family = AF_INET,   // use AF_INET and AF_INET6 for iPv4 and iPv6 respectively
		.ai_socktype = SOCK_STREAM, // use TCP type
		.ai_flags = AI_PASSIVE, 
	};
	if ((status = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo error: %s", gai_strerror(status));
		exit(1);
	}
	if ((sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)) == -1) {
		perror("socket"); // _fd for file descriptor
	}
	/* fun fact:
	 * PF_INET == AF_INET, but P is protocol and A is address. 
	 * Difference is that Address Family (AF) might support several protocols were reffered by their Protocol Family (PF)
	 * */
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof yes)) {
		perror("setsockopt");
	}
	if (bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) < 0) {
		perror("bind failed");
	}
	freeaddrinfo(servinfo);
	if (listen(sockfd, 10) < 0) {
		perror("listen");
	}
	return sockfd;
}

USER_INFO init_user(int sockfd, int room_number, char *username) {
	USER_INFO user = {
		.sockfd = sockfd,
		.room_number = room_number,
	};
	strcpy(user.username, username);
	user.username[strlen(username) - 1] = '\0';
	return user;
}

void init_users() {
	for (int i = 0; i < MAX_CLIENTS; i++) {
		users[i].sockfd = -1;
	}
}

void client_access(int sockfd) {
	init_users();

	struct sockaddr_storage their_addr;
	socklen_t addr_size = sizeof their_addr;
	char recv_buf[BUFFER_SIZE];

	int new_fd;
	int i = 0;
	while (1) {
		if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size)) == -1) {
			perror("accept");
		}
		memset(recv_buf, 0, BUFFER_SIZE);
		recv(new_fd, recv_buf, BUFFER_SIZE, 0);
		if (is_room_correct(recv_buf)) {
			int room_number = atoi(recv_buf);
			char username[20] = {'\0'};
			recv(new_fd, username, 20, 0);
			users[i] = init_user(new_fd, room_number, username);
			printf("[Connected] Client(%d) %s\n",
					users[i].sockfd, users[i].username);
			pthread_create(&thread_clients[i], NULL, client_connection, &(users[i]));
			i++;
		} else {
			char *error_message = "[Error] Room number is incorrect\n";
			send(new_fd, error_message, strlen(error_message), 0);
			close(new_fd);
		}
		sleep(1);
	}
}

void client_close(int sockfd) {
	for (int i = 0; i < 100; i++) {
		if (users[i].sockfd == sockfd) {
			users[i].sockfd = -1;
			printf("[Disconnected] Client(%d) %s\n", users[i].sockfd, users[i].username);
			break;
		}
	}
	close(sockfd);
}

void sendtoroom(char *message, int room_number) {	

}
