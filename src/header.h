#include <pthread.h>
#include <stdatomic.h>

#define NUMBER_PARTICIPANTS 8
#define RESPONSE_SIZE 130
#define MSG_SIZE 101 // \n character considered
#define NAME_SIZE 21 // \n character considered

typedef struct client_info
{
    int client_socket;
    FILE *write;
    _Atomic int present;
    pthread_mutex_t lock; // for concurent writing to the socket
} client_info;

client_info clients[NUMBER_PARTICIPANTS];

_Atomic int thread_counter;

int handleConnection(int client_id);

void handleRequest(client_info client);

void die(char *message);
void error(char *message);
