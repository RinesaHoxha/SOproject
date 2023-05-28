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

void handle_message_passing() {
    pthread_t threads[MAX_CLIENTS];

    while (1) {
        struct Request request;

        // Receive client request from the message queue
        msgrcv(message_queue, &request, sizeof(struct Request) - sizeof(long), 0, 0);

        // Acquire the mutex to handle the client request
        pthread_mutex_lock(&mutex);

        // Check if the maximum number of clients is reached
        if (client_count < MAX_CLIENTS) {
            // Create a new thread to handle the client request
            pthread_create(&threads[client_count], NULL, handle_client, (void*) &request);

            // Increase client count
            client_count++;
        } else {
            printf("Maximum number of clients reached. Rejecting client %ld\n", request.client_id);
            // Handle the rejection or send an appropriate response to the client
            // ...
        }
    }
}

void handle_shared_memory() {
    while (1) {
        // Wait for the client request
        while (shared_memory->client_id == 0) {
            sleep(1);
        }

        // Acquire the mutex to handle the client request
        pthread_mutex_lock(&mutex);

        // Check if the maximum number of clients is reached
        if (client_count < MAX_CLIENTS) {
            // Create a copy of the client request
            struct Request request;
            request.client_id = shared_memory->client_id;
            strncpy(request.message, shared_memory->message, MAX_MESSAGE_SIZE);

            // Reset the shared memory
            shared_memory->client_id = 0;
            memset(shared_memory->message, 0, MAX_MESSAGE_SIZE);

            // Create a new thread to handle the client request
            pthread_t thread;
            pthread_create(&thread, NULL, handle_client, (void*) &request);

            // Increase client count
            client_count++;
        } else {
            // Handle the rejection or send an appropriate response to the client
            // ...
        }
    }
}

void handle_pipes() {
    while (1) {
        int pipe_fd[2];

        // Create a pipe for client communication
        if (pipe(pipe_fd) == -1) {
            perror("Failed to create pipe");
            exit(EXIT_FAILURE);
        }

        // Fork a child process to handle the client request
        pid_t pid = fork();

        if (pid == -1) {
            perror("Failed to fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // Child process

            // Close the write end of the pipe
            close(pipe_fd[1]);

            struct Request request;

            // Read client request from the pipe
            read(pipe_fd[0], &request, sizeof(struct Request));

            // Handle the client request
            handle_client(&request);

            // Close the read end of the pipe
            close(pipe_fd[0]);

            // Exit the child process
            exit(EXIT_SUCCESS);
        } else {
            // Parent process

            // Close the read end of the pipe
            close(pipe_fd[0]);

            struct Response response;

            // Prepare server response
            printf("Enter your response: ");
            fgets(response.message, MAX_MESSAGE_SIZE, stdin);

            // Write server response to the pipe
            write(pipe_fd[1], &response, sizeof(struct Response));

            // Close the write end of the pipe
            close(pipe_fd[1]);

            // Wait for the child process to finish
            wait(NULL);
        }
    }
}
