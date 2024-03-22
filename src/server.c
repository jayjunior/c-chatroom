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
#include "logger.h"

void *run(void *);

static void init_logger(void);
static void log(char state, char *action, ...);

void die(char *message)
{
    perror(message);
    exit(EXIT_FAILURE);
}
static void init_logger(void)
{

    if (remove("../build/logs_server.txt") == -1)
    {
        die("remove");
    }
    FILE *log = fopen("../build/logs_server.txt", "a");
    if (log == NULL)
    {
        die("fopen");
    }
    log_add_fp(log, LOG_TRACE);
}

void error(char *message)
{
    perror(message);
}
static void log(char state, char *action, ...)
{
    switch (state)
    {
    case 'p':
        log_info("Setting up %s", action);
        break;
    case 's':
        log_info("Succesfully %s", action);
        break;
    case 'e':
        log_error("Error %s", action);
        break;
    case 'w':
        log_warn("Couldn't %s", action);
        break;
    default:
        log_warn("Unknown state %s", action);
        break;
    }
}

int main(int argc, char **argv)
{
    init_logger();
    log('s', "initialized logger");

    int listen_sock = socket(AF_INET, SOCK_STREAM, 0);

    unsigned short PORT = 2345;

    // zero indexed

    thread_counter = ATOMIC_VAR_INIT(0);

    // -1 means client doens't yet exist

    for (int i = 0; i < NUMBER_PARTICIPANTS; i++)
    {
        clients[i].client_socket = -1;
        clients[i].present = 0;
    }
    log('s', "initialized clients storage stuctures");

    if (listen_sock == -1)
    {
        log('e', "creating listening socket");
        die("socket");
    }
    log('s', "created listening socket");

    struct sockaddr_in connection_configurations;
    struct in_addr address;
    address.s_addr = INADDR_ANY;
    connection_configurations.sin_family = AF_INET;
    connection_configurations.sin_port = htons(PORT);
    connection_configurations.sin_addr = address;

    log('s', "set up connection configurations");
    int flag = 1;
    if (setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)) == -1)
    {
        log('w', "set up setsockopt");
        error("setsockopt");
    }

    if (bind(listen_sock, (struct sockaddr *)&connection_configurations, sizeof(connection_configurations)) == -1)
    {
        log('e', "binding listen socket");
        die("bind");
    }

    if (listen(listen_sock, SOMAXCONN) == -1)
    {
        log('e', "listening on socket");
        die("listen");
    }

    struct sigaction action = {
        .sa_handler = SIG_IGN,
        .sa_flags = SA_RESTART,
    };

    if (sigemptyset(&(action.sa_mask)) == -1)
    {
        log('w', "create sigemptyset");
        error("sigemptyset");
    }

    if (sigaction(SIGPIPE, &action, NULL) == -1)
    {
        log('w', "create sigaction for SIGPIPE");
        error("sigaction");
    }

    while (1)
    {
        int client_sock = accept(listen_sock, NULL, NULL);

        if (client_sock == -1)
        {
            log('w', "accept incoming client connection");
            error("accept");
            continue;
        }
        log('s', "accepted client connection for client %d", client_sock);
        if (atomic_load(&thread_counter) == NUMBER_PARTICIPANTS)
        {
            if (close(client_sock) == -1)
            {
                log('w', "close client socket");
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
                log('s', "found free spot in list for client %d", client_sock);
                break;
            }
            pthread_mutex_unlock(&clients[i].lock);
        }
        if (spot == -1)
        {
            log('w', "find free spot in list for client %d", client_sock);
            continue;
        }

        clients[spot].client_socket = client_sock;
        log('s', "set client socket %d in list", client_sock);
        if (handleConnection(spot) == EXIT_SUCCESS)
        {
            pthread_mutex_lock(&clients[spot].lock);
            clients[spot].present = 1;
            log('s', "set client %d as present", client_sock);
            pthread_mutex_unlock(&clients[spot].lock);
            atomic_fetch_add(&thread_counter, 1);
            log('s', "incremented thread counter");
        }
    }

    return EXIT_SUCCESS;
}
