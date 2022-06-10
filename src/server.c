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
#include <ctype.h>

#include <unistd.h>
#include <pthread.h>

#define PORT "8765"


FILE *database;
pthread_t thread_clients[100];
typedef struct {
	char username[20];
	int room_number;
	int sockfd;
} USER_INFO;


void *client_connection(void *vargp) {
	USER_INFO *user = vargp;
	char buffer[65536] = {'\0'};
	while (1) {
		memset(buffer, 0, sizeof buffer);
		recv(user->sockfd, buffer, 65536, 0);
		if (strcmp(buffer, "exit\n") == 0) {
			printf("[Disconnected] Client(%d) %s \n", user->sockfd, user->username);
			close(user->sockfd);
			break;
		} 
		printf("%s", buffer);
	}
	pthread_exit(NULL);
}

int is_room_correct(char *buffer) {
	printf("%s", buffer);
	for (int i = 0; i < strlen(buffer) - 2; i++) {
		if (!isdigit(buffer[i])) {
			return 0;
		}
	}
	return 1;
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

void init_user(USER_INFO user, int sockfd, int room_number, char *username) {
	// user.sockfd = sockfd;
	// user.room_number = room_number;
	// strcpy(user.username, username);
	// user.username[strlen(username) - 1] = '\0';
	return;
}

void client_access(int sockfd) {
	struct sockaddr_storage their_addr;
	socklen_t addr_size = sizeof their_addr;
	char recv_buf[65536];
	USER_INFO user[100];

	int new_fd;
	int i = 0;
	while (1) {
		if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size)) == -1) {
			perror("accept");
		}
		recv(new_fd, recv_buf, sizeof recv_buf, 0);
		if (is_room_correct(recv_buf)) {
			int room_number = atoi(recv_buf);
			memset(recv_buf, 0, sizeof recv_buf);
			recv(new_fd, recv_buf, sizeof recv_buf, 0);
			init_user(user[i], new_fd, room_number, recv_buf);
			pthread_create(&thread_clients[i++], NULL, client_connection, user);
		} else {
			char *error_message = "[Error] Room number is incorrect\n";
			send(new_fd, error_message, strlen(error_message), 0);
			close(new_fd);
		}
		sleep(1);
	}
}

int main(void) {
	int sockfd = init_server();

	client_access(sockfd);

	return 0;
}
