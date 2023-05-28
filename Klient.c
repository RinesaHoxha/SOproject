#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>

// Constants
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

// Function prototypes
void send_message_passing();
void send_shared_memory();
void send_pipes();

int main() {
    // Get the key for the message queue and shared memory
    key_t key = ftok(".", 's');

    // Send client requests using different IPC mechanisms
    send_message_passing(key);
    send_shared_memory(key);
    send_pipes();

    return 0;
}

void send_message_passing(key_t key) {
    int message_queue = msgget(key, 0666);
    if (message_queue == -1) {
        perror("Failed to access message queue");
        exit(EXIT_FAILURE);
    }

    struct Request request;
    struct Response response;

    while (1) {
        // Prepare client request
        request.client_id = getpid();
        printf("Enter your message (or 'exit' to quit): ");
        fgets(request.message, MAX_MESSAGE_SIZE, stdin);

        // Remove the newline character from the input
        request.message[strcspn(request.message, "\n")] = '\0';

        // Check if the user wants to exit
        if (strcmp(request.message, "exit") == 0)
            break;

        // Send client request to the message queue
        msgsnd(message_queue, &request, sizeof(struct Request) - sizeof(long), 0);

        // Receive server response from the message queue
        msgrcv(message_queue, &response, sizeof(struct Response) - sizeof(long), request.client_id, 0);

        // Print server response
        printf("Server response: %s\n", response.message);
    }
}

void send_shared_memory(key_t key) {
    int shmid = shmget(key, sizeof(struct Request), 0666);
    if (shmid == -1) {
        perror("Failed to access shared memory");
        exit(EXIT_FAILURE);
    }

    struct Request* shared_memory = (struct Request*) shmat(shmid, NULL, 0);
    if (shared_memory == (void*) -1) {
        perror("Failed to attach shared memory");
        exit(EXIT_FAILURE);
    }

    while (1) {
        // Prepare client request
        shared_memory->client_id = getpid();
        printf("Enter your message (or 'exit' to quit): ");
        fgets(shared_memory->message, MAX_MESSAGE_SIZE, stdin);

        // Remove the newline character from the input
        shared_memory->message[strcspn(shared_memory->message, "\n")] = '\0';

        // Check if the user wants to exit
        if (strcmp(shared_memory->message, "exit") == 0)
            break;

        // Wait for the server response
        while (shared_memory->client_id != 0) {
            sleep(1);
        }

        // Print server response
        printf("Server response: %s\n", shared_memory->message);
    }
    // Detach shared memory
    shmdt(shared_memory);
}

void send_pipes() {
    int pipe_fd[2];

    // Create a pipe for client communication
    if (pipe(pipe_fd) == -1) {
        perror("Failed to create pipe");
        exit(EXIT_FAILURE);
    }

    // Fork a child process to handle the server response
    pid_t pid = fork();

    if (pid == -1) {
        perror("Failed to fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Child process

        // Close the write end of the pipe
        close(pipe_fd[1]);

        struct Response response;

        // Read server response from the pipe
        read(pipe_fd[0], &response, sizeof(struct Response));

        // Print server response
        printf("Server response: %s", response.message);
