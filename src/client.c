#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <pwd.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>
#include <signal.h>

#define SERVER_ADDRESS "141.145.210.99"
#define PORT "2345"
#define HOSTNAME_LENGTH 255
#define RESPONSE_SIZE 129
#define MSG_SIZE 101
#define NAME_SIZE 21

static void die(char *message)
{
    perror(message);
    exit(EXIT_FAILURE);
}

static void error(char *message)
{
    fprintf(stderr, "Error Client function ");
    perror(message);
}

static void display_prompt(void)
{
    fprintf(stdout, "=>");
    fflush(stdout);
}

static void remove_prompt(void)
{
    fprintf(stdout, "\n");
    fflush(stdout);
}
static void *read_from_user_and_send(void *args)
{

    char msg[MSG_SIZE];
    char name[NAME_SIZE];

    FILE *write = (FILE *)args;


    if (fprintf(stdout, "%s", "What's your name ?: ") < 0)
    {
        error("fprintf");
    }

    fflush(stdout);

    fgets(name, sizeof(name) + 1, stdin);

    if (ferror(stdin))
    {
        error("fgets");
    }

    fflush(stdin);

    if (fprintf(write, "%s", name) < 0)
    {
        error("fprintf");
    }

    fflush(write);

    display_prompt();

    while (1)
    {

        fgets(msg, sizeof(msg) + 1, stdin);
        if (ferror(stdin))
        {
            error("fgets");
        }
        fflush(stdin);

        if (fprintf(write, "%s", msg) == EOF)
        {
            error("fprintf");
        }
        fflush(write);
        display_prompt();
    }

    return NULL;
}

static void *read_from_server_and_display(void *args)
{
    int *sock = args;
    char response[RESPONSE_SIZE];
    int recv_status;
    while (1)
    {
        /*
            Some message residues from previous messages are left in the response\pipeline
        */
        memset(response, 0, sizeof(response));

        recv_status = recv(*sock, response, sizeof(response), 0);

        if (recv_status == 0)
        {
            fprintf(stdout, "%s", "\nConnection closed or rejected retry later \n");
            exit(EXIT_SUCCESS);
        }
        if (recv_status == -1)
            continue;

        remove_prompt();
        fprintf(stdout, "%s", response);
        fflush(stdout);
        display_prompt();
    }

    return NULL;
}

int main(int argc, char **argv)
{


    char host_name[HOSTNAME_LENGTH];
    if (gethostname(host_name, HOSTNAME_LENGTH) == -1)
    {
        die("gethostname");
    }

    // DNS-LOOKUP
    struct addrinfo hints = {
        .ai_socktype = SOCK_STREAM,
        .ai_family = AF_UNSPEC,
        .ai_flags = AI_ADDRCONFIG,
        .ai_canonname = host_name,
        .ai_protocol = 0};

    struct addrinfo *head;
    int ecode = getaddrinfo(SERVER_ADDRESS, PORT, &hints, &head);

    const char *addrinfo_error = gai_strerror(ecode);

    if (ecode != 0)
    {
        fprintf(stderr, "%s\n", addrinfo_error);
    }

    struct addrinfo *curr;

    int sock;

    for (curr = head; curr != NULL; curr = curr->ai_next)
    {
        sock = socket(curr->ai_family, curr->ai_socktype, curr->ai_protocol); // let's get the socket
        if (sock < 0)
        {
            error("socket");
        }

        // let's try connecting to server using our socket
        if (connect(sock, curr->ai_addr, curr->ai_addrlen) == 0)
        {
            break;
        }
        continue;
    }

    if (curr == NULL)
    {
        die("socket");
    }

    FILE *write;

    if ((write = fdopen(sock, "w")) == NULL)
    {
        error("write");
        if (close(sock) == -1)
        {
            error("close");
        }
        exit(EXIT_FAILURE);
    }

    int read_socket;

    if ((read_socket = dup(sock)) == -1)
    {
        error("dup");

        if (fclose(write) == EOF)
        {
            error("fclose");
        }
        exit(EXIT_FAILURE);
    }

    FILE *read;

    if ((read = fdopen(read_socket, "r")) == NULL)
    {
        error("read");

        if (fclose(write) == EOF)
        {
            error("fclose");
        }

        if (close(read_socket) == -1)
        {
            error("close");
        }
    }


    char *welcome_msg = "\n\
░██╗░░░░░░░██╗███████╗██╗░░░░░░█████╗░░█████╗░███╗░░░███╗███████╗  ████████╗░█████╗░  ████████╗██╗░░██╗███████╗\n\
░██║░░██╗░░██║██╔════╝██║░░░░░██╔══██╗██╔══██╗████╗░████║██╔════╝  ╚══██╔══╝██╔══██╗  ╚══██╔══╝██║░░██║██╔════╝\n\
░╚██╗████╗██╔╝█████╗░░██║░░░░░██║░░╚═╝██║░░██║██╔████╔██║█████╗░░  ░░░██║░░░██║░░██║  ░░░██║░░░███████║█████╗░░\n\
░░████╔═████║░██╔══╝░░██║░░░░░██║░░██╗██║░░██║██║╚██╔╝██║██╔══╝░░  ░░░██║░░░██║░░██║  ░░░██║░░░██╔══██║██╔══╝░░\n\
░░╚██╔╝░╚██╔╝░███████╗███████╗╚█████╔╝╚█████╔╝██║░╚═╝░██║███████╗  ░░░██║░░░╚█████╔╝  ░░░██║░░░██║░░██║███████╗\n\
░░░╚═╝░░░╚═╝░░╚══════╝╚══════╝░╚════╝░░╚════╝░╚═╝░░░░░╚═╝╚══════╝  ░░░╚═╝░░░░╚════╝░  ░░░╚═╝░░░╚═╝░░╚═╝╚══════╝\n\
\n\
████████╗██████╗░██╗██╗░░░░░░█████╗░░██████╗░██╗░░░██╗  ░█████╗░███████╗\n\
╚══██╔══╝██╔══██╗██║██║░░░░░██╔══██╗██╔════╝░╚██╗░██╔╝  ██╔══██╗██╔════╝\n\
░░░██║░░░██████╔╝██║██║░░░░░██║░░██║██║░░██╗░░╚████╔╝░  ██║░░██║█████╗░░\n\
░░░██║░░░██╔══██╗██║██║░░░░░██║░░██║██║░░╚██╗░░╚██╔╝░░  ██║░░██║██╔══╝░░\n\
░░░██║░░░██║░░██║██║███████╗╚█████╔╝╚██████╔╝░░░██║░░░  ╚█████╔╝██║░░░░░\n\
░░░╚═╝░░░╚═╝░░╚═╝╚═╝╚══════╝░╚════╝░░╚═════╝░░░░╚═╝░░░  ░╚════╝░╚═╝░░░░░\n\
\n\
████████╗██████╗░██╗░░░██╗████████╗██╗░░██╗\n\
╚══██╔══╝██╔══██╗██║░░░██║╚══██╔══╝██║░░██║\n\
░░░██║░░░██████╔╝██║░░░██║░░░██║░░░███████║\n\
░░░██║░░░██╔══██╗██║░░░██║░░░██║░░░██╔══██║\n\
░░░██║░░░██║░░██║╚██████╔╝░░░██║░░░██║░░██║\n\
░░░╚═╝░░░╚═╝░░╚═╝░╚═════╝░░░░╚═╝░░░╚═╝░░╚═╝\n\n";

    char *banner = "=============================================== CHAT ============================================================\n";

    fprintf(stdout, "%s", welcome_msg);

    fprintf(stdout, "%s", banner);

    fflush(stdout);

    // create two different threads , for read and write

    pthread_t id;
    errno = 0;
    if ((errno = pthread_create(&id, NULL, read_from_user_and_send, write)) != 0)
    {
        die("pthread_create");
    }

    errno = 0;
    if ((errno = pthread_create(&id, NULL, read_from_server_and_display, &sock)) != 0)
    {
        die("pthread_create");
    }

    while (1)
    {
        sleep(30);
    }
}
