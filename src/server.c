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

FILE *datatbase;
pthread_t thread_clients[100];

void *client_connection(void *vargp) {
	int clientfd = *(int *)vargp;
	char buffer[65536];
	printf("Client %d connected\n", clientfd);
	recv(clientfd, buffer, 65536, 0);
	printf("%s", buffer);


	return NULL;
}


int main(void) {
	int status, sockfd, new_fd, yes = 1; 
	datatbase = fopen("./userdata.txt", "a");
	struct addrinfo *servinfo;
	struct addrinfo hints = {
		.ai_family = AF_INET,   // no metter iPv6 of iPv4. use AF_INET and AF_INET6 for iPv4 and iPv6 respectively
		.ai_socktype = SOCK_STREAM, // make TCP type
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
	struct sockaddr_storage their_addr;
	socklen_t addr_size = sizeof their_addr;
	char recv_buf[65536];

	int i = 0;
	while (1) {
		if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size)) == -1) {
			perror("accept");
		}
		recv(new_fd, recv_buf, sizeof recv_buf, 0);
		printf("%s", recv_buf);
		pthread_create(&thread_clients[i++], NULL, client_connection, (void *)&new_fd);
		sleep(1);
	}
	fclose(datatbase);
	return 0;
}
