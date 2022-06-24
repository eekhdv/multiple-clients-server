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

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <pthread.h>
#include <unistd.h>

#include "init.h"
#include "utils.h"

int init_server()
{
    int status, sockfd, yes = 1;
    struct addrinfo* servinfo;
    struct addrinfo hints = {
        .ai_family = AF_INET, // use AF_INET and AF_INET6 for iPv4 and iPv6 respectively
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
     * Difference is that Address Family (AF) might support several protocols
     * were reffered by their Protocol Family (PF)
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

USER_INFO* init_users()
{
    USER_INFO* users = malloc(sizeof *users * MAX_CLIENTS);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        users[i].sockfd = -1;
        memset(users[i].username, 0, 20);
    }
    return users;
}

ROOM* init_rooms()
{
    ROOM* rooms = malloc(sizeof *rooms * MAX_ROOMS);
    for (int i = 0; i < MAX_ROOMS; i++) {
        rooms[i].room_number = rooms[i].users_limit = -1;
        rooms[i].members_num = rooms[i].room_exist = 0;
    }
    return rooms;
}

USER_INFO init_user(int sockfd, int room_number, char* username)
{
    USER_INFO user = {
        .sockfd = sockfd,
        .room_number = room_number,
    };
    strcpy(user.username, username);
    user.username[strlen(username)] = '\0';
    return user;
}

pthread_t* init_clients_thread()
{
    pthread_t* thread_clients = malloc(sizeof *thread_clients * MAX_CLIENTS);
    return thread_clients;
}
