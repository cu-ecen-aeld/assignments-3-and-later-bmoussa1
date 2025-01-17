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

#define PORT 9000
#define BACKLOG 10
#define FILE_PATH "/var/tmp/aesdsocketdata"

// File descriptors for the server and client sockets
int server_fd, client_fd;
int running = 1;

// Signal handler for SIGINT and SIGTERM
void signal_handler(int signum) {
    printf("Caught signal %d, exiting\n\r", signum);
    fflush(stdout); // Force flushing of stdout
    syslog(LOG_INFO, "Caught signal, exiting");
    running = 0;
    close(server_fd);
    if (client_fd > 0) close(client_fd);
    remove(FILE_PATH);
    exit(0);
}

// Handles communication with a connected client
void handle_client(int client_socket) {
    char buffer[1024];
    ssize_t bytes_received;
    int file_fd = open(FILE_PATH, O_CREAT | O_APPEND | O_RDWR, 0644);

    if (file_fd < 0) {
        syslog(LOG_ERR, "Failed to open file %s", FILE_PATH);
        close(client_socket);
        return;
    }

    while ((bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes_received] = '\0'; // Null-terminate the received data
        write(file_fd, buffer, bytes_received); // Append data to the file

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
    syslog(LOG_INFO, "Closed connection from %s", inet_ntoa(((struct sockaddr_in *)&client_socket)->sin_addr));
    close(client_socket);
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
        printf("Binding failed\n\r");
        fflush(stdout); // Force flushing of stdout
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
        printf("Listening failed\n\r");
        fflush(stdout); // Force flushing of stdout
        close(server_fd);
        return -1;
    }

    while (running) {
        // Accept a connection from a client
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addrlen);
        if (client_fd < 0) {
            if (!running) break; // Exit loop if the server is shutting down
            printf("Accept failed\n\r");
            fflush(stdout); // Force flushing of stdout
            syslog(LOG_ERR, "Accept failed");
            continue;
        }
	
	printf("Accepted connection from %s\n\r", inet_ntoa(client_addr.sin_addr));
	fflush(stdout); // Force flushing of stdout
        syslog(LOG_INFO, "Accepted connection from %s", inet_ntoa(client_addr.sin_addr));
        handle_client(client_fd); // Handle communication with the client
    }

    close(server_fd);
    closelog(); // Close connection to syslog
    return 0;
}

