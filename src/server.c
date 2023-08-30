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
#include <signal.h>
#include <stdbool.h>
#define DEFINE_GLOBALS
#include "header.h"

// TODO cross platfomr builds

void *run(void *);

void die(char *message)
{
    perror(message);
    exit(EXIT_FAILURE);
}

void error(char *message)
{
    fprintf(stderr, "Error Server function ");
    perror(message);
}

int main(int argc, char **argv)
{
    int listen_sock = socket(AF_INET6, SOCK_STREAM, 0);

    unsigned short PORT = 2345;

    // zero indexed

    thread_counter = ATOMIC_VAR_INIT(0);

    // -1 means client doens't yet exist

    for (int i = 0; i < NUMBER_PARTICIPANTS; i++)
    {
        clients[i].client_socket = -1;
        clients[i].present = 0;
    }

    if (listen_sock == -1)
    {
        die("socket");
    }

    struct sockaddr_in6 connection_configurations = {
        .sin6_family = AF_INET6,
        .sin6_port = htons(PORT),
        .sin6_addr = in6addr_any};

    int flag = 1;
    if (setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)) == -1)
    {
        error("setsockopt");
    }

    if (bind(listen_sock, (struct sockaddr *)&connection_configurations, sizeof(connection_configurations)) == -1)
    {
        die("bind");
    }

    if (listen(listen_sock, SOMAXCONN) == -1)
    {
        die("listen");
    }

    struct sigaction action = {
        .sa_handler = SIG_IGN,
        .sa_flags = SA_RESTART,
    };

    if (sigemptyset(&(action.sa_mask)) == -1)
    {
        error("sigemptyset");
    }

    if (sigaction(SIGPIPE, &action, NULL) == -1)
    {
        error("sigaction");
    }

    while (1)
    {

        int client_sock = accept(listen_sock, NULL, NULL);

        if (client_sock == -1)
        {
            error("accept");
            continue;
        }

        int participants = NUMBER_PARTICIPANTS;
        if (atomic_load(&thread_counter) == NUMBER_PARTICIPANTS)
        {
            if (close(client_sock) == -1)
            {
                error("close");
            }
        }

        // search for free spot in list
        int spot = -1;
        for (int i = 0; i < NUMBER_PARTICIPANTS; i++)
        {
            pthread_mutex_lock(&clients[i].lock);
            if (clients[i].present == 0)
            {
                spot = i;
                break;
            }
            pthread_mutex_unlock(&clients[i].lock);
        }
        if (spot == -1)
        {
            continue;
        }

        clients[spot].client_socket = client_sock;
        if (handleConnection(spot) == EXIT_SUCCESS)
        {
            pthread_mutex_lock(&clients[spot].lock);
            clients[spot].present = 1;
            pthread_mutex_unlock(&clients[spot].lock);
            atomic_fetch_add(&thread_counter, 1);
        }
    }

    return EXIT_SUCCESS;
}
