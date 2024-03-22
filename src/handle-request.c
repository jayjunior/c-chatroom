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
#include "logger.h"

void *run(void *args)
{

    errno = pthread_detach(pthread_self());

    if (errno)
    {
        log_warn("couldn't detach thread");
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
        log_warn("Couldn't open write stream for client %d", client_socket);
        return NULL;
    }
    client->write = write;
    log_info("Opened write stream for client %d", client_socket);
    pthread_mutex_init(&client->lock, NULL);

    handleRequest(*client);

    if (fclose(write) == EOF)
    {
        error("fclose");
    }
    atomic_fetch_sub(&thread_counter, 1);
    int count = atomic_load(&thread_counter);
    log_info("Successfully decremented thread counter to %d", count);
    pthread_mutex_lock(&client->lock);
    client->present = 0;
    log_info("Successfully reset present bit for client %d", client_socket);
    pthread_mutex_unlock(&client->lock);
    client->client_socket = -1;
    log_info("Successfully reset client socket for client %d", client_socket);
    pthread_mutex_destroy(&client->lock);
    fflush(stdout);
    return NULL;
}

void send_to_other_users(char *response, client_info client, client_info clients[])
{

    for (int i = 0; i < NUMBER_PARTICIPANTS; i++)
    {
        pthread_mutex_lock(&clients[i].lock);
        if (clients[i].present == 1)
        {
            if (clients[i].client_socket != client.client_socket)
            {
                if (fprintf(clients[i].write, "%s", response) == EOF)
                {
                    log_warn("Couldn't send message %s to client %d", response, clients[i].client_socket);
                    error("fprintf");
                }
                else
                {
                    log_info("successfully sent message %s to client %d", response, clients[i].client_socket);
                }
                fflush(clients[i].write);
            }
        }
        pthread_mutex_unlock(&clients[i].lock);
    }
}
void handleRequest(client_info client)
{

    char name[NAME_SIZE];
    char msg[MSG_SIZE];
    char response[RESPONSE_SIZE];
    int recv_status;

    recv_status = recv(client.client_socket, name, sizeof(name), 0);
    if (recv_status == 0)
    {
        log_warn("Client %d disconnected", client.client_socket);
        return;
    }
    if (recv_status == -1)
    {
        log_warn("Couldn't receive name from client %d ... assigned default name", client.client_socket);
        snprintf(name, sizeof(name), "User-%d\n", client.client_socket);
    }
    name[strlen(name) - 1] = '\x0';
    log_info("successfully assigned name %s to client %d", name, client.client_socket);

    // inform other participants about new user , if any .

    if (snprintf(response, sizeof(response), "%s entered the chat\n", name) < 0)
    {
        log_warn("Couldn't create response for client %d", client.client_socket);
        error("snprintf");
    }
    send_to_other_users(response, client, clients);

    while (1)
    {
        /*
            Some message residues from previous messages are left in the response\pipeline , msg\pipeline
        */
        memset(response, 0, sizeof(response));
        memset(msg, 0, sizeof(msg));

        recv_status = recv(client.client_socket, msg, sizeof(msg), 0);

        if (recv_status == 0)
        {
            snprintf(response, sizeof(response), "%s left the chat\n", name);

            send_to_other_users(response, client, clients);

            return;
        }
        if (recv_status == -1)
        {
            log_warn("Couldn't receive message from client %d", client.client_socket);
            error("recv");
            continue;
        }
        msg[strlen(msg) - 1] = '\x0';
        if (snprintf(response, sizeof(response), "%s : %s\n", name, msg) < 0)
        {
            log_warn("Couldn't create response for client %d", client.client_socket);
            error("snprintf");
        }
        fflush(stdout);

        send_to_other_users(response, client, clients);
    }
}
