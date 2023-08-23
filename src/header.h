#include <pthread.h>
#include <stdatomic.h>

#define NUMBER_PARTICIPANTS 8
#define RESPONSE_SIZE 132
#define MSG_SIZE 102 // \n and null byte character considered
#define NAME_SIZE 22 // \n character considered

typedef struct client_info
{
    int client_socket;
    FILE *write;
    _Atomic int present;
    pthread_mutex_t lock; // for concurent writing to the socket
} client_info;

/* list for storing connected clients */
client_info clients[NUMBER_PARTICIPANTS];

_Atomic int thread_counter;

int handleConnection(int client_id);

void handleRequest(client_info client);

void die(char *message);
void error(char *message);
