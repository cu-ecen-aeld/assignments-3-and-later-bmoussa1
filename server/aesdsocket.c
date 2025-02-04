#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <syslog.h>
#include <pthread.h>
#include <time.h>

#define PORT 9000
#define BACKLOG 10
#define FILE_PATH "/var/tmp/aesdsocketdata"
#define TIMESTAMP_INTERVAL 10 // Seconds

// Mutex for synchronizing file writes
pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;

int server_fd, running = 1;
pthread_t timestamp_thread;  // Thread for periodic timestamp writing
pthread_t *client_threads = NULL;  // Array of client threads
int num_threads = 0;  // Number of client threads

// Signal handler for SIGINT and SIGTERM
void signal_handler(int signum) {
    printf("Caught signal %d, exiting\n\r", signum);
    fflush(stdout); // Force flushing of stdout
    syslog(LOG_INFO, "Caught signal, exiting");
    running = 0;
    close(server_fd);
    remove(FILE_PATH);
    
    // Wait for all threads to finish
    for (int i = 0; i < num_threads; i++) {
        pthread_join(client_threads[i], NULL);
    }
    
    // Clean up client thread array
    free(client_threads);

    pthread_cancel(timestamp_thread);  // Cancel the timestamp thread
    pthread_mutex_destroy(&file_mutex);  // Destroy the mutex
    exit(0);
}

// Function to append timestamp to the file every 10 seconds
void* append_timestamp(void* arg) {
    while (running) {
        sleep(TIMESTAMP_INTERVAL);

        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        char timestamp[100];
        strftime(timestamp, sizeof(timestamp), "%a, %d %b %Y %H:%M:%S %z", tm_info);

        pthread_mutex_lock(&file_mutex);
        int file_fd = open(FILE_PATH, O_CREAT | O_APPEND | O_RDWR, 0644);
        if (file_fd < 0) {
            syslog(LOG_ERR, "Failed to open file %s", FILE_PATH);
            pthread_mutex_unlock(&file_mutex);
            continue;
        }
        write(file_fd, "timestamp:", 10);
        write(file_fd, timestamp, strlen(timestamp));
        write(file_fd, "\n", 1);
        close(file_fd);
        pthread_mutex_unlock(&file_mutex);
    }
    return NULL;
}

// Handles communication with a connected client
void* handle_client(void* arg) {
    int client_socket = *(int*)arg;
    free(arg); // Free the dynamically allocated client socket
    char buffer[1024];
    ssize_t bytes_received;
    int file_fd = open(FILE_PATH, O_CREAT | O_APPEND | O_RDWR, 0644);

    if (file_fd < 0) {
        syslog(LOG_ERR, "Failed to open file %s", FILE_PATH);
        close(client_socket);
        return NULL;
    }

    while ((bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes_received] = '\0'; // Null-terminate the received data

        pthread_mutex_lock(&file_mutex);
        write(file_fd, buffer, bytes_received); // Append data to the file
        pthread_mutex_unlock(&file_mutex);

        // Check if the received data contains a newline
        if (strchr(buffer, '\n')) {
            lseek(file_fd, 0, SEEK_SET); // Reset file pointer to the beginning
            while ((bytes_received = read(file_fd, buffer, sizeof(buffer) - 1)) > 0) {
                send(client_socket, buffer, bytes_received, 0); // Send file contents to the client
            }
            lseek(file_fd, 0, SEEK_END); // Reset file pointer to the end for appending
        }
    }

    close(file_fd);
    close(client_socket);
    return NULL;
}

// Daemonize the program
void daemonize() {
    pid_t pid = fork();
    if (pid < 0) {
        syslog(LOG_ERR, "Fork failed");
        exit(1);
    } else if (pid > 0) {
        exit(0); // Parent process exits, child runs in background
    }

    // Create a new session and detach from terminal
    if (setsid() < 0) {
        syslog(LOG_ERR, "Failed to create a new session");
        exit(1);
    }

    // Fork again to ensure the daemon cannot acquire a controlling terminal
    pid = fork();
    if (pid < 0) {
        syslog(LOG_ERR, "Second fork failed");
        exit(1);
    } else if (pid > 0) {
        exit(0); // Parent process exits
    }

    // Redirect stdout and stderr to /dev/null
    freopen("/dev/null", "w", stdout);
    freopen("/dev/null", "w", stderr);
}

int main(int argc, char *argv[]) {
    struct sockaddr_in server_addr, client_addr;
    socklen_t addrlen = sizeof(client_addr);
    int daemon_mode = 0;
    int opt;

    // Parse command line arguments to check for daemon entry
    while ((opt = getopt(argc, argv, "d")) != -1) {
        switch (opt) {
            case 'd':
                daemon_mode = 1; // Enable daemon mode
                break;
            default:
                fprintf(stderr, "Usage: %s [-d]\n", argv[0]);
                exit(1);
        }
    }

    openlog("aesdsocket", LOG_PID | LOG_CONS, LOG_USER); // Open connection to syslog
    signal(SIGINT, signal_handler); // Register signal handler for SIGINT
    signal(SIGTERM, signal_handler); // Register signal handler for SIGTERM

    // Create a socket for the server
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        syslog(LOG_ERR, "Socket creation failed");
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind the socket to the specified port
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        syslog(LOG_ERR, "Binding failed");
        close(server_fd);
        return -1;
    }

    // If in daemon mode, fork and detach
    if (daemon_mode) {
        daemonize();
    }

    // Start listening for incoming connections
    if (listen(server_fd, BACKLOG) < 0) {
        syslog(LOG_ERR, "Listening failed");
        close(server_fd);
        return -1;
    }

    // Start the timestamp thread
    pthread_create(&timestamp_thread, NULL, append_timestamp, NULL);

    while (running) {
        // Accept a connection from a client
        int *client_socket = malloc(sizeof(int));
        *client_socket = accept(server_fd, (struct sockaddr *)&client_addr, &addrlen);
        if (*client_socket < 0) {
            if (!running) break; // Exit loop if the server is shutting down
            syslog(LOG_ERR, "Accept failed");
            continue;
        }

        printf("Accepted connection from %s\n\r", inet_ntoa(client_addr.sin_addr));
        syslog(LOG_INFO, "Accepted connection from %s", inet_ntoa(client_addr.sin_addr));

        // Reallocate memory for the new thread array
        num_threads++;
        client_threads = realloc(client_threads, num_threads * sizeof(pthread_t));

        // Create a new thread to handle the client connection
        pthread_create(&client_threads[num_threads - 1], NULL, handle_client, client_socket);
    }

    // Wait for all client threads to finish before exiting
    for (int i = 0; i < num_threads; i++) {
        pthread_join(client_threads[i], NULL);
    }

    // Wait for the timestamp thread to finish before exiting
    pthread_join(timestamp_thread, NULL);

    close(server_fd);
    free(client_threads);  // Free memory for client threads array
    closelog(); // Close connection to syslog
    return 0;
}

