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
#include "header.h"
#include "logger.h"

void *run(void *);

static void init_logger(void);
static void die(char *message);

static void die(char *message)
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

int main(int argc, char **argv)
{
    init_logger();
    log_info("Succesfully initialized logger");

    int listen_sock = socket(AF_INET, SOCK_STREAM, 0);

    unsigned short PORT = 2345;

    // zero indexed

    thread_counter = ATOMIC_VAR_INIT(0);

    // -1 means client doens't yet exist

    for (int i = 0; i < NUMBER_PARTICIPANTS; i++)
    {
        log_info("Setting up client sockets and present bits %d", i);
        clients[i].client_socket = -1;
        clients[i].present = 0;
    }

    if (listen_sock == -1)
    {
        log_error("Error creating listening socket");
        die("socket");
    }
    log_info("Succesfully created listening socket");

    struct sockaddr_in connection_configurations;
    struct in_addr address;
    address.s_addr = INADDR_ANY;
    connection_configurations.sin_family = AF_INET;
    connection_configurations.sin_port = htons(PORT);
    connection_configurations.sin_addr = address;

    log_info("Succesfully created connection configurations");
    int flag = 1;
    if (setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)) == -1)
    {
        log_warn("setsockopt failed");
        error("setsockopt");
    }

    if (bind(listen_sock, (struct sockaddr *)&connection_configurations, sizeof(connection_configurations)) == -1)
    {
        log_error("couldn't bind socket");
        die("bind");
    }

    if (listen(listen_sock, SOMAXCONN) == -1)
    {
        log_error("Error listening on socket");
        die("listen");
    }

    struct sigaction action = {
        .sa_handler = SIG_IGN,
        .sa_flags = SA_RESTART,
    };

    if (sigemptyset(&(action.sa_mask)) == -1)
    {
        log_warn("Error creating sigemptyset");
        error("sigemptyset");
    }

    if (sigaction(SIGPIPE, &action, NULL) == -1)
    {
        log_warn("Error creating sigaction for SIGPIPE");
        error("sigaction");
    }

    while (1)
    {
        int client_sock = accept(listen_sock, NULL, NULL);

        if (client_sock == -1)
        {
            log_warn("Error accepting client connection");
            error("accept");
            continue;
        }

        if (atomic_load(&thread_counter) == NUMBER_PARTICIPANTS)
        {
            if (close(client_sock) == -1)
            {
                log_warn("Error closing client socket");
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
