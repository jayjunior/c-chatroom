#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <inttypes.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>
#include "logger.h"
#define IN_CONNECTION
#include "header.h"
void *run(void *);

int handleConnection(int client_id)
{

    // trigger a thread to take care of the interaction
    int client_sock = clients[client_id].client_socket;

    pthread_t id;
    errno = 0;
    if ((errno = pthread_create(&id, NULL, run, &clients[client_id])) != 0)
    {
        fprintf(stderr, "Couldn't create thread for client %d", client_sock);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
