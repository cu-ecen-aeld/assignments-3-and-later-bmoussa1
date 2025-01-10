#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>
#include <unistd.h>

// Function to write a string to a file
int write_to_file(const char *file_path, const char *write_str) {
    FILE *file = fopen(file_path, "w"); // Open in write mode to create/truncate
    if (file == NULL) {
        syslog(LOG_ERR, "Error opening file %s: %s", file_path, strerror(errno));
        return -1;
    }

    // Write the string to the file
    if (fprintf(file, "%s\n", write_str) < 0) {
        syslog(LOG_ERR, "Error writing to file %s: %s", file_path, strerror(errno));
        fclose(file); // Attempt to close even if write fails
        return -1;
    }

    // Ensure data is written to disk
    if (fclose(file) != 0) {
        syslog(LOG_ERR, "Error closing file %s: %s", file_path, strerror(errno));
        return -1;
    }

    syslog(LOG_DEBUG, "Successfully wrote '%s' to %s", write_str, file_path);
    return 0;
}

int main(int argc, char *argv[]) {
    const char *file_path;
    const char *write_str;

    // Open syslog with LOG_USER facility
    openlog("writer", LOG_PID | LOG_CONS, LOG_USER);

    // Check the number of arguments
    if (argc != 3) {
        syslog(LOG_ERR, "Invalid number of arguments. Usage: %s <file_path> <write_string>", argv[0]);
        fprintf(stderr, "Usage: %s <file_path> <write_string>\n", argv[0]);
        closelog();
        return 1;
    }

    file_path = argv[1];
    write_str = argv[2];

    // Write the string to the file
    if (write_to_file(file_path, write_str) != 0) {
        fprintf(stderr, "Failed to write to file %s\n", file_path);
        closelog();
        return 1;
    }

    // Close syslog
    closelog();
    return 0;
}

