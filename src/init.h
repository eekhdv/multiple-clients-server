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
