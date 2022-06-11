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
#include <semaphore.h>

#include "server.h"
#include "utils.h"

#define PORT "8765"
#define BUFFER_SIZE 65536
#define MAX_CLIENTS 100
#define MAX_ROOMS 100

sem_t x;
pthread_t thread_clients[MAX_CLIENTS];
USER_INFO users[MAX_CLIENTS];
ROOM rooms[MAX_ROOMS];

void *client_connection(void *vargp) {
	USER_INFO user = *(USER_INFO *)vargp;
	char buffer[BUFFER_SIZE] = {'\0'};
	while (1) {
		memset(buffer, 0, BUFFER_SIZE);
		int nbytes = recv(user.sockfd, buffer, BUFFER_SIZE, 0);
		if (strcmp(buffer, "/exit\n") == 0 || nbytes <= 0) {
			client_close(user.sockfd);
			break;
		} 
		sendtoroom(buffer, user.username, user.room_number, user.sockfd);
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
		memset(users[i].username, 0, 20);
	}
}

void init_rooms() {
	for (int i = 0; i < MAX_ROOMS; i++) {
		rooms[i].room_number = rooms[i].users_limit = -1;
	}
}

void ask_username(int sockfd, char username[20]) {
	char askusername[] = "Enter your username: ";
	send(sockfd, askusername, strlen(askusername), 0);
	recv(sockfd, username, 20, 0);
}


int create_room(unsigned long long room_number) {
	int room_place = -1;
	for (int i = 0; i < MAX_ROOMS; i++) {
		if (rooms[i].room_number == room_number) {
			return i;
		} else {
			if (room_place == rooms[i].room_number) {
				room_place = i;
				rooms[i].room_number = room_number;
			}
		}
	}
	return room_place;
}

void client_access(int sockfd) {
	init_users();
	init_rooms();

	struct sockaddr_storage their_addr;
	socklen_t addr_size = sizeof their_addr;
	char recv_buf[BUFFER_SIZE];

	sem_init(&x, 0, 1);

	int new_fd;
	int usr = 0;
	while (1) {
		if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size)) == -1) {
			perror("accept");
		}
		sem_wait(&x);
		char askroom[] = "Enter room number: ";
		send(new_fd, askroom, strlen(askroom), 0);
		memset(recv_buf, 0, BUFFER_SIZE);
		recv(new_fd, recv_buf, BUFFER_SIZE, 0);
		if (is_room_correct(recv_buf)) {
			int roo = create_room(atoi(recv_buf));
			char username[20] = {'\0'};

			ask_username(new_fd, username);
			users[usr] = init_user(new_fd, rooms[roo].room_number, username);
			printf("[Connected] Client(%d) %s\n",
					users[usr].sockfd, users[usr].username);

			pthread_create(&thread_clients[usr], NULL, client_connection, &(users[usr]));
			sendtoroom("<- connected to the room ...\n", users[usr].username, rooms[roo].room_number, new_fd);
			sem_post(&x);

			usr++;
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
			printf("[Disconnected] Client(%d) %s\n", sockfd, users[i].username);
			sendtoroom("<- left the room ...\n", users[i].username, users[i].room_number, sockfd);
			break;
		}
	}
	close(sockfd);
}

void sendtoroom(char *message, char *sendername, int room_number, int sendersfd) {	
	for (int i = 0; i < MAX_CLIENTS; i++) {
		if (users[i].room_number == room_number && users[i].sockfd != -1 && users[i].sockfd != sendersfd) {
			char senders_name[30] = {'\0'};
			senders_name[0] = '[';
			strcat(senders_name, sendername);
			strcat(senders_name, "] ");
			send(users[i].sockfd, senders_name, strlen(senders_name), 0);
			send(users[i].sockfd, message, strlen(message), 0);
		}
	}
}
