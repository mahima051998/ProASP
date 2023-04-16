#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <stdbool.h>
#include<fcntl.h>

#define PORT 3000
#define BUFFER_SIZE 1024

// This function takes a string input as a command and processes it, returning the result as a string.
char* process_command(char* input) {
    // Allocate memory for the result string and create an array to store command arguments.
    char* result = (char*)malloc(sizeof(char) * 1024);
    char* args[2] = {NULL, NULL};

    // If the command matches the "findfile" pattern, find the file and store the result in the result string.
    if (sscanf(input, "findfile %ms", &args[0]) == 1) {
        sprintf(result, "output=$(find ~/ -maxdepth 1 -type f -name %s -exec stat -c '%%s\\t%%w\\t%%n' {} \\; | head -n 1); if [ -z \"$output\" ]; then echo \"File not found\"; else echo -e \"$output\"; fi", args[0]);
        free(args[0]);
    } 
    // If the command matches the "sgetfiles" pattern, get files based on their size range and store the result in the result string.
    else if (sscanf(input, "sgetfiles %ms %ms%*c", &args[0], &args[1]) == 2) {
        snprintf(result, 1024, "zip temp.tar.gz $(find ~/ -maxdepth 1 -type f -size +%sc -size -%sc)", args[0], args[1]);
        free(args[0]);
        free(args[1]);
    } 
    // If the command matches the "dgetfiles" pattern, get files modified between two dates and store the result in the result string.
    else if (sscanf(input, "dgetfiles %ms %ms%*c", &args[0], &args[1]) == 2) {
        snprintf(result, 1024, "zip temp.tar.gz $(find ~/ -maxdepth 1 -type f -newermt \"%s\" ! -newermt \"%s\")", args[0], args[1]);
        free(args[0]);
        free(args[1]);
    } 
    // If the command matches the "getfiles" pattern, get files based on a list of file names and store the result in the result string.
    else if (sscanf(input, "getfiles %[^\n]", result) == 1) {
        char files[1024] = "";
        char* token = strtok(result, " ");
        int count = 0;
        while (token != NULL) {
            char tmp[256];
            if (count > 0) {
                strcat(files, "-o ");
            }
            snprintf(tmp, 256, "-name %s ", token);
            strcat(files, tmp);
            count++;
            token = strtok(NULL, " ");
        }
        sprintf(result, "zip temp.tar.gz $(find ~/ -maxdepth 1 -type f %s)", files);
    } 
    // If the command matches the "gettargz" pattern, get files based on their extension and store the result in the result string.
    else if (sscanf(input, "gettargz %[^\n]", result) == 1) {
        char files[1024] = "";
        // Use strtok to tokenize the "result" string by space
        char* token = strtok(result, " ");
        int count = 0;
        // Loop through each token
        while (token != NULL) {
            // Initialize a temporary character array to store the current file name
            char tmp[256];
            // If this is not the first file name, add "-o" to the "files" array
            if (count > 0) {
                strcat(files, "-o ");
            }
            // Use snprintf to create the command for finding files with the current extension
            snprintf(tmp, 256, "-iname \"*.%s\" ", token);
            // Append the current command to the "files" array
            strcat(files, tmp);
            count++;
             // Move on to the next token
            token = strtok(NULL, " ");
        }
        // Use sprintf to create the command for zipping files with the specified extensions
        sprintf(result, "zip temp.tar.gz $(find ~/ -maxdepth 1 -type f %s)", files);
    } 
    // Check if the input is the "quit" command
    else if (strcmp(input, "quit") == 0) {
        // Exit the program with status code 0
        exit(0);
    } 
    // If the input is not a recognized command
    else {
        // Use snprintf to create an error message
        snprintf(result, 1024, "Invalid Command");
    }
    // Return the result string
    return result;
}

int main() {
    // Create a socket for the client
    int client_socket;
    struct sockaddr_in server_address;

    // Create a TCP socket with IPv4 address family and default protocol (0)
    client_socket = socket(AF_INET, SOCK_STREAM, 0);

    // Set up the server address with the specified port number and IP address
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(3000);
    server_address.sin_addr.s_addr = inet_addr("127.0.0.1");
    memset(server_address.sin_zero, 0, sizeof(server_address.sin_zero));

    // Connect to the server using the client socket and server address
    if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        perror("connect failed");
        exit(EXIT_FAILURE);
    }

    // Set the client socket to non-blocking mode
    int socket_flags = fcntl(client_socket, F_GETFL, 0);
    fcntl(client_socket, F_SETFL, socket_flags | O_NONBLOCK);

    // Enter the main loop for sending and receiving data
    while (1) {
        // Prepare buffers for user input and server output
        char user_input[1024];
        char server_output[1024];
        char *validated_input;

        // Prompt the user to enter a command
        printf("Enter command: ");

        // Read user input from standard input
        fgets(user_input, 1024, stdin);

        // Flush standard input to clear any remaining characters
        fflush(stdin);

        // Remove trailing newline character from user input
        if (user_input[strlen(user_input) - 1] == '\n') {
            user_input[strlen(user_input) - 1] = '\0';
        }

        // Call the validate function to validate the user input
        validated_input = process_command(user_input);

        // Send the validated command to the server using the client socket
        send(client_socket, validated_input, strlen(validated_input), 0);

        // Use select to check for data received from the server
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(client_socket, &read_fds);
        struct timeval timeout;
        timeout.tv_sec = 1; // Set the timeout value in seconds
        timeout.tv_usec = 0;

        int select_result = select(client_socket + 1, &read_fds, NULL, NULL, &timeout);

        if (select_result == -1) {
            perror("select failed");
            exit(EXIT_FAILURE);
        } else if (select_result == 0) {
            // No data received from the server, continue to the next iteration
            continue;
        } else {
            // Receive data from the server and print it to the console
            memset(server_output, 0, sizeof(server_output));
            while (recv(client_socket, server_output, sizeof(server_output), 0) > 0) {
                printf("%s", server_output);
                memset(server_output, 0, sizeof(server_output));
            }
        }
    }

    // Close the client socket
    close(client_socket);

    return 0;
}

