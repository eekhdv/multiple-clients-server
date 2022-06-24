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

struct user_start {
    int new_fd;
    SERVER* serv;
};

struct client_conn {
    USER_INFO user;
    SERVER* serv;
};

void* client_connection(void* vargp)
{
    struct client_conn conn_thread = *(struct client_conn*)vargp;
    char buffer[BUFFER_SIZE] = { '\0' };
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int nbytes = recv(conn_thread.user.sockfd, buffer, BUFFER_SIZE, 0);
        if (strcmp(buffer, "/exit\n") == 0 || nbytes <= 0) {
            client_close(conn_thread.serv, conn_thread.user.sockfd);
            break;
        }
        sendtoroom(conn_thread.serv, buffer, conn_thread.user.username, conn_thread.user.room_number, conn_thread.user.sockfd);
    }
    pthread_exit(NULL);
}

void ask_username(int sockfd, char username[20])
{
    char askusername[] = "Enter your username: ";
    send(sockfd, askusername, strlen(askusername), 0);
    recv(sockfd, username, 20, 0);
}

int ask_limit(SERVER* serv, int room, int sockfd)
{
    char new_room_wow[] = "Wow! There doesn't seem to be such a room! I'll create one...\n";
    send(sockfd, new_room_wow, strlen(new_room_wow), 0);

    char asklim[] = "Enter the max number of members in the room (max 999): \n";
    send(sockfd, asklim, strlen(asklim), 0);

    char room_limit[3] = { '\0' };
    recv(sockfd, room_limit, 3, 0);
    if (is_uint(room_limit)) {
        serv->rooms[room].users_limit = atoi(room_limit);
        return 1;
    } else {
        char* error_message = "[Error] Max number is incorrect\n";
        send(sockfd, error_message, strlen(error_message), 0);
        close(sockfd);
        return 0;
    }
}

int have_places(SERVER* serv, int room_id)
{
    return serv->rooms[room_id].users_limit != serv->rooms[room_id].members_num;
}

int create_room(SERVER* serv, unsigned long long room_number, int sockfd)
{
    int room_place = -1;
    for (int i = 0; i < MAX_ROOMS; i++) {
        if (serv->rooms[i].room_number == room_number) {
            if (have_places(serv, i)) {
                return i;
            }
            char* error_message = "[Error] Room is full :(\n";
            send(sockfd, error_message, strlen(error_message), 0);
            close(sockfd);
            return -1;
        }
        if (room_place == -1 && !serv->rooms[i].room_exist) {
            room_place = i;
            serv->rooms[i].room_number = room_number;
        }
    }
    if (!ask_limit(serv, room_place, sockfd)) {
        return -1;
    }
    return room_place;
}

void server_run(SERVER* serv, int sockfd)
{
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
        struct user_start user_thread = {
            .new_fd = new_fd,
            .serv = serv,
        };
        pthread_create(&thread_start_user[start_usr++], NULL, start_user_thread, &(user_thread));

        sleep(1);
    }
}

int get_room_id(SERVER* serv, int room_number)
{
    for (int i = 0; i < MAX_ROOMS; i++) {
        if ((int)serv->rooms[i].room_number == room_number) {
            return i;
        }
    }
    return -1;
}

int get_client_id(SERVER* serv, int sockfd)
{
    for (int i = 0; i < 100; i++) {
        if (serv->users[i].sockfd == sockfd) {
            return i;
        }
    }
    return -1;
}

void client_close(SERVER* serv, int sockfd)
{
    int client_id = get_client_id(serv, sockfd);
    serv->users[client_id].sockfd = -1;
    int room_id = get_room_id(serv, serv->users[client_id].room_number);
    serv->rooms[room_id].members_num--;
    printf("[Disconnected] Client(%d) %s\n", sockfd, serv->users[client_id].username);
    printf("[ROOM] %llu : %d/%d\n", serv->rooms[room_id].room_number, serv->rooms[room_id].members_num, serv->rooms[room_id].users_limit);
    if (serv->rooms[room_id].members_num == 0) {
        printf("Room #%llu is empty, so deleting the room.\n", serv->rooms[room_id].room_number);
        serv->rooms[room_id].room_exist = 0;
    }
    sendtoroom(serv, "<- left the room ...\n", serv->users[client_id].username, serv->users[client_id].room_number, sockfd);
    close(sockfd);
}

char* message_with_sendername(char* sendername, char* message)
{
    char* new_message = calloc(BUFFER_SIZE + 30, sizeof(char));
    strcat(new_message, sendername);
    new_message[strlen(new_message) - 1] = ' '; /* last char in sendername is '\n', so "-1" */
    strcat(new_message, message);
    return new_message;
}

void sendtoroom(SERVER* serv, char* message, char* sendername, int room_number, int sendersfd)
{
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (serv->users[i].room_number == room_number && serv->users[i].sockfd != -1 && serv->users[i].sockfd != sendersfd) {
            char* new_message = message_with_sendername(sendername, message);
            send(serv->users[i].sockfd, new_message, strlen(new_message), 0);
            free(new_message);
        }
    }
}

void* start_user_thread(void* vargp)
{
    struct user_start user_thread = *(struct user_start*)vargp;
    send(user_thread.new_fd, "Enter room number: ", 20, 0);

    char recv_buf[BUFFER_SIZE] = { '\0' };
    recv(user_thread.new_fd, recv_buf, BUFFER_SIZE, 0);
    if (is_uint(recv_buf)) {
        int roo = create_room(user_thread.serv, atoi(recv_buf), user_thread.new_fd);
        if (roo == -1) {
            pthread_exit(NULL);
        }
        user_thread.serv->rooms[roo].room_exist = 1;
        char username[20] = { '\0' };

        ask_username(user_thread.new_fd, username);
        user_thread.serv->users[user_thread.serv->usr] = init_user(user_thread.new_fd, user_thread.serv->rooms[roo].room_number, username);
        struct client_conn conn_thread = {
            .user = user_thread.serv->users[user_thread.serv->usr],
            .serv = user_thread.serv,
        };
        pthread_create(&user_thread.serv->thread_clients[user_thread.serv->usr], NULL, client_connection, &(conn_thread));
        user_thread.serv->rooms[roo].members_num++;

        printf("[Connected] Client(%d) %s\n", user_thread.serv->users[user_thread.serv->usr].sockfd, user_thread.serv->users[user_thread.serv->usr].username);
        printf("[ROOM] %llu : %d/%d\n", user_thread.serv->rooms[roo].room_number, user_thread.serv->rooms[roo].members_num, user_thread.serv->rooms[roo].users_limit);
        sendtoroom(user_thread.serv, "<- connected to the room ...\n", user_thread.serv->users[user_thread.serv->usr].username, user_thread.serv->rooms[roo].room_number, user_thread.new_fd);

        user_thread.serv->usr++;
    } else {
        printf("%s", recv_buf);
        char* error_message = "[Error] Room number is incorrect\n";
        send(user_thread.new_fd, error_message, strlen(error_message), 0);
        close(user_thread.new_fd);
    }
    pthread_exit(NULL);
}

SERVER* create_server(int sockfd)
{
    SERVER* serv = malloc(sizeof *serv);
    serv->thread_clients = init_clients_thread();
    serv->rooms = init_rooms();
    serv->users = init_users();
    serv->serverfd = sockfd;
    return serv;
}
