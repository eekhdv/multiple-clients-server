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

void *client_connection(void *vargp) {
	int clientfd = *(int *)vargp;
	char buffer[65536] = {'\0'};
	while (1) {
		memset(buffer, 0, sizeof buffer);
		recv(clientfd, buffer, 65536, 0);
		if (strcmp(buffer, "exit\n") == 0) {
			printf("Client %d disconnected\n", clientfd);
			close(clientfd);
			break;
		} 
		printf("%s", buffer);
	}
	return NULL;
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

void client_access(int sockfd) {
	struct sockaddr_storage their_addr;
	socklen_t addr_size = sizeof their_addr;
	char recv_buf[65536];

	int new_fd;
	int i = 0;
	while (1) {
		if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size)) == -1) {
			perror("accept");
		}
		recv(new_fd, recv_buf, sizeof recv_buf, 0);
		if (is_room_correct(recv_buf)) {
			printf("[Connected] Client %d\n", new_fd);
			pthread_create(&thread_clients[i++], NULL, client_connection, (void *)&new_fd);
		} else {
			char *error_message = "[Error] Room number is incorrect";
			send(new_fd, error_message, strlen(error_message), 0);
			close(new_fd);
		}
		sleep(1);
	}
}

int main(void) {
	database = fopen("./userdata.txt", "a");
	int sockfd = init_server();

	client_access(sockfd);

	fclose(database);
	return 0;
}
