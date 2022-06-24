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

#pragma once
#include <pthread.h>

#define PORT "8765"
#define BUFFER_SIZE 65536
#define MAX_CLIENTS 100
#define MAX_ROOMS 100

typedef struct {
    char username[20];
    int room_number;
    int sockfd;
} USER_INFO;

typedef struct {
    unsigned long long room_number;
    int users_limit;
    int members_num;
    short room_exist;
} ROOM;

typedef struct {
    pthread_t *thread_clients;
    USER_INFO *users;
    ROOM *rooms;
	int serverfd;
	int usr;
} SERVER;

int init_server();
ROOM* init_rooms();
USER_INFO* init_users();
pthread_t *init_clients_thread();
USER_INFO init_user(int sockfd, int room_number, char* username);
