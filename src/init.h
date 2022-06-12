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

#include <pthread.h>

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

#define PORT "8765"
#define BUFFER_SIZE 65536
#define MAX_CLIENTS 100
#define MAX_ROOMS 100

int init_server();
void init_rooms(ROOM rooms[]);
void init_users(USER_INFO users[]);
USER_INFO init_user(int sockfd, int room_number, char *username);
