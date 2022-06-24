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
#include "server.h"
#include "utils.h"

pthread_t thread_clients[MAX_CLIENTS];
USER_INFO users[MAX_CLIENTS];
ROOM rooms[MAX_ROOMS];
int usr;

void* client_connection(void* vargp)
{
    USER_INFO user = *(USER_INFO*)vargp;
    char buffer[BUFFER_SIZE] = { '\0' };
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

void ask_username(int sockfd, char username[20])
{
    char askusername[] = "Enter your username: ";
    send(sockfd, askusername, strlen(askusername), 0);
    recv(sockfd, username, 20, 0);
}

int ask_limit(int room, int sockfd)
{
    char new_room_wow[] = "Wow! There doesn't seem to be such a room! I'll create one...\n";
    send(sockfd, new_room_wow, strlen(new_room_wow), 0);

    char asklim[] = "Enter the max number of members in the room (max 999): \n";
    send(sockfd, asklim, strlen(asklim), 0);

    char room_limit[3] = { '\0' };
    recv(sockfd, room_limit, 3, 0);
    if (is_uint(room_limit)) {
        rooms[room].users_limit = atoi(room_limit);
        return 1;
    } else {
        char* error_message = "[Error] Max number is incorrect\n";
        send(sockfd, error_message, strlen(error_message), 0);
        close(sockfd);
        return 0;
    }
}

int have_places(int room_id)
{
    return rooms[room_id].users_limit != rooms[room_id].members_num;
}

int create_room(unsigned long long room_number, int sockfd)
{
    int room_place = -1;
    for (int i = 0; i < MAX_ROOMS; i++) {
        if (rooms[i].room_number == room_number) {
            if (have_places(i)) {
                return i;
            }
            char* error_message = "[Error] Room is full :(\n";
            send(sockfd, error_message, strlen(error_message), 0);
            close(sockfd);
            return -1;
        }
        if (room_place == -1 && !rooms[i].room_exist) {
            room_place = i;
            rooms[i].room_number = room_number;
        }
    }
    if (!ask_limit(room_place, sockfd)) {
        return -1;
    }
    return room_place;
}

void client_access(int sockfd)
{
    init_users(users);
    init_rooms(rooms);

    struct sockaddr_storage their_addr;
    socklen_t addr_size = sizeof their_addr;

    int new_fd;
    int start_usr = 0;
    pthread_t thread_start_user[MAX_CLIENTS];
    while (1) {
        if ((new_fd = accept(sockfd, (struct sockaddr*)&their_addr, &addr_size)) == -1) {
            perror("accept");
            continue;
        }
        pthread_create(&thread_start_user[start_usr++], NULL, start_user_thread, &(new_fd));

        sleep(1);
    }
}

int get_room_id(int room_number)
{
    for (int i = 0; i < MAX_ROOMS; i++) {
        if ((int)rooms[i].room_number == room_number) {
            return i;
        }
    }
    return -1;
}

int get_client_id(int sockfd)
{
    for (int i = 0; i < 100; i++) {
        if (users[i].sockfd == sockfd) {
            return i;
        }
    }
    return -1;
}

void client_close(int sockfd)
{
    int client_id = get_client_id(sockfd);
    users[client_id].sockfd = -1;
    int room_id = get_room_id(users[client_id].room_number);
    rooms[room_id].members_num--;
    printf("[Disconnected] Client(%d) %s\n", sockfd, users[client_id].username);
    printf("[ROOM] %llu : %d/%d\n", rooms[room_id].room_number, rooms[room_id].members_num, rooms[room_id].users_limit);
    if (rooms[room_id].members_num == 0) {
        printf("Room #%llu is empty, so deleting the room.\n", rooms[room_id].room_number);
        rooms[room_id].room_exist = 0;
    }
    sendtoroom("<- left the room ...\n", users[client_id].username, users[client_id].room_number, sockfd);
    close(sockfd);
}

void sendtoroom(char* message, char* sendername, int room_number, int sendersfd)
{
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (users[i].room_number == room_number && users[i].sockfd != -1 && users[i].sockfd != sendersfd) {
            char senders_name[30] = { '\0' };
            senders_name[0] = '[';
            strcat(senders_name, sendername);
            strcat(senders_name, "] ");
            send(users[i].sockfd, senders_name, strlen(senders_name), 0);
            send(users[i].sockfd, message, strlen(message), 0);
        }
    }
}

void* start_user_thread(void* vargp)
{
    int new_fd = *(int*)vargp;
    char recv_buf[BUFFER_SIZE];
    char askroom[] = "Enter room number: ";
    send(new_fd, askroom, strlen(askroom), 0);
    memset(recv_buf, 0, BUFFER_SIZE);
    recv(new_fd, recv_buf, BUFFER_SIZE, 0);
    if (is_uint(recv_buf)) {
        int roo = create_room(atoi(recv_buf), new_fd);
        if (roo == -1) {
            pthread_exit(NULL);
        }
        rooms[roo].room_exist = 1;
        char username[20] = { '\0' };

        ask_username(new_fd, username);
        users[usr] = init_user(new_fd, rooms[roo].room_number, username);
        pthread_create(&thread_clients[usr], NULL, client_connection, &(users[usr]));
        rooms[roo].members_num++;
        printf("[Connected] Client(%d) %s\n", users[usr].sockfd, users[usr].username);
        printf("[ROOM] %llu : %d/%d\n", rooms[roo].room_number, rooms[roo].members_num, rooms[roo].users_limit);
        sendtoroom("<- connected to the room ...\n", users[usr].username, rooms[roo].room_number, new_fd);

        usr++;
    } else {
        printf("%s", recv_buf);
        char* error_message = "[Error] Room number is incorrect\n";
        send(new_fd, error_message, strlen(error_message), 0);
        close(new_fd);
    }
    pthread_exit(NULL);
}
