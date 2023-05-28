#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <sys/msg.h>
#include <sys/shm.h>

// Constants
#define MAX_CLIENTS 5
#define MAX_MESSAGE_SIZE 100

// Structure for client request
struct Request {
    long client_id;
    char message[MAX_MESSAGE_SIZE];
};

// Structure for server response
struct Response {
    long client_id;
    char message[MAX_MESSAGE_SIZE];
};

// Global variables
struct Request* shared_memory;
pthread_mutex_t mutex;
int message_queue;
int client_count; // Track the number of clients being served
// Function prototypes
void* handle_client(void* arg);
void handle_message_passing();
void handle_shared_memory();
void handle_pipes();

int main() {
    // Initialize synchronization mechanisms
    pthread_mutex_init(&mutex, NULL);

    // Create message queue
    key_t key = ftok(".", 's');
    message_queue = msgget(key, IPC_CREAT | 0666);
    if (message_queue == -1) {
        perror("Failed to create message queue");
        exit(EXIT_FAILURE);
    }

    // Create shared memory
    int shmid = shmget(key, sizeof(struct Request), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("Failed to create shared memory");
        exit(EXIT_FAILURE);
    }

    // Attach shared memory
    shared_memory = (struct Request*) shmat(shmid, NULL, 0);
    if (shared_memory == (void*) -1) {
        perror("Failed to attach shared memory");
        exit(EXIT_FAILURE);
    }
    // Initialize client count
    client_count = 0;

    // Handle client requests using different IPC mechanisms
    handle_message_passing();
    handle_shared_memory();
    handle_pipes();

    // Cleanup
    shmdt(shared_memory);
    shmctl(shmid, IPC_RMID, NULL);
    msgctl(message_queue, IPC_RMID, NULL);
    pthread_mutex_destroy(&mutex);

    return 0;
}

void* handle_client(void* arg) {
    struct Request* request = (struct Request*) arg;
    struct Response response;

    // Echo back the client's request
    response.client_id = request->client_id;
    strncpy(response.message, request->message, MAX_MESSAGE_SIZE);

    // Simulate some processing time
    sleep(1);

    // Send the response back to the client
    msgsnd(message_queue, &response, sizeof(struct Response) - sizeof(long), 0);
 // Release the mutex
    pthread_mutex_unlock(&mutex);

    printf("Sent response to client %ld: %s\n", response.client_id, response.message);

    return NULL;
}
