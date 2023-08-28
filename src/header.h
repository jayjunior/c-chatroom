#ifdef DEFINE_GLOBALS
#define GLOBAL
#else // !DEFINE_GLOBALS
#define GLOBAL extern
#endif

#include <pthread.h>
#include <stdatomic.h>

#define NUMBER_PARTICIPANTS 8
#define RESPONSE_SIZE 132
#define MSG_SIZE 102 /** \n and null byte character considered */
#define NAME_SIZE 22 /** \n character considered */

/** @brief client information structure */
typedef struct client_info
{
    int client_socket;    /**< socket obtained during connection from accept() */
    FILE *write;          /**< File Descriptor for writing to client_sock */
    _Atomic int present;  /**< is a thread already attributed to the client */
    pthread_mutex_t lock; /**< client lock for concurrent writing */
} client_info;

/** @brief list for storing connected clients */
GLOBAL client_info clients[NUMBER_PARTICIPANTS];

/** @brief Atomic counter keeping track of current amount of connected clients/threads */
GLOBAL _Atomic int thread_counter;

/**
 *   @brief creates a new running thread for the connection initiated by @c client_id
 *
 *   @param client_id
 *
 *   @retval @c 0 on Success
 *   @retval @c 1 Otherwise
 */
int handleConnection(int client_id);

/**
 *   @brief handle communication bewtwenn @c client and the server over a thread
 *   previously created in @c handleConnection
 *
 *   @param client
 */
void handleRequest(client_info client);

/**
 *   @brief exit process with @c EXIT_FAILURE and prints @c message to the @c stderr
 *
 *   @param message
 */
void die(char *message);
/**
 * @brief prints @c message to the @c stderr without exiting
 *
 * @param message
 */
void error(char *message);
