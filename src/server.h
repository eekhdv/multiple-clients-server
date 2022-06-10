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

typedef struct {
	char username[20];
	int room_number;
	int sockfd;
} USER_INFO;

void *client_connection(void *vargp);
void client_close(int sockfd);
void client_access(int sockfd);

void sendtoroom(char *message, int room_number);

int init_server();
USER_INFO init_user(int sockfd, int room_number, char *username);
