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
#include "header.h"

/* only returns on error */

void *run(void *args)
{

    errno = pthread_detach(pthread_self());

    if (errno)
    {
        error("pthread_detach");
    }

    client_info *client = args;

    // from every socket create two File pointers;

    int client_socket = client->client_socket;

    FILE *write;

    if ((write = fdopen(client_socket, "w")) == NULL)
    {
        fprintf(stderr, "Couldn't open write stream\n");
        if (close(client_socket) == -1)
        {
            fprintf(stderr, "Couldn't close client socket\n");
        }
        return NULL;
    }
    client->write = write;

    pthread_mutex_init(&client->lock, NULL);

    handleRequest(*client);

    if (fclose(write) == EOF)
    {
        error("fclose");
    }

    atomic_fetch_sub(&thread_counter, 1);
    atomic_store(&client->present, 0);

    fflush(stdout);
    return NULL;
}

void send_to_other_users(char *response, client_info client, client_info clients[])
{
    for (int i = 0; i < NUMBER_PARTICIPANTS; i++)
    {
        // TODO: Possible race conditions between thread and main thread
        if (clients[i].client_socket != -1 && clients[i].client_socket != client.client_socket)
        {

            pthread_mutex_lock(&clients[i].lock);

            if (fprintf(clients[i].write, "%s", response) == EOF)
            {
                error("fprintf");
            }

            fflush(clients[i].write);
            pthread_mutex_unlock(&clients[i].lock);
        }
    }
}
void handleRequest(client_info client)
{

    char name[NAME_SIZE];
    char msg[MSG_SIZE];
    char response[RESPONSE_SIZE];

#pragma region get_name

    if (recv(client.client_socket, name, sizeof(name), 0) == 0)
    {
        // TODO: left chat
    }
    name[strlen(name) - 1] = '\x0';

#pragma endregion get_name

    // inform other participants about new user , if any .
    // TODO: size passed to snprintf ???
    snprintf(response, sizeof(response), "%s entered the chat\n", name);
    send_to_other_users(response, client, clients);

    while (1)
    {
        /*
            Some message residues from previous messages are left in the response\pipeline , msg\pipeline
        */
        memset(response, 0, sizeof(response));
        memset(msg, 0, sizeof(msg));

        if (recv(client.client_socket, msg, sizeof(msg), 0) == 0)
        {
            snprintf(response, sizeof(response), "%s left the chat\n", name);

            send_to_other_users(response, client, clients);
            pthread_mutex_destroy(&client.lock);
            return;
        }
        msg[strlen(msg) - 1] = '\x0';

        snprintf(response, sizeof(response), "%s : %s\n", name, msg);
        fflush(stdout);
        send_to_other_users(response, client, clients);
    }
}
