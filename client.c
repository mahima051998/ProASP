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
        sprintf(result, 1024, "output=$(find ~/ -maxdepth 1 -type f -name %s -exec stat -c '%%s\\t%%w\\t%%n' {} \\; | head -n 1); if [ -z \"$output\" ]; then echo \"File not found\"; else echo -e \"$output\"; fi", res[0]);
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
    // Create a socket
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Specify the server address and port
    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Connect to the server
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect failed");
        exit(EXIT_FAILURE);
    }

    // Send and receive data in a loop
    while (true) {
        char command[BUFFER_SIZE];
        char output[BUFFER_SIZE];
        char *result;

        // Get user input
        printf("Enter command: ");
        fgets(command, BUFFER_SIZE, stdin);
        if (command[strlen(command) - 1] == '\n') {
            command[strlen(command) - 1] = '\0';
        }

        // Process the command and send it to the server
        result = process_command(command);
        send(client_socket, result, strlen(result), 0);

        // Receive and print the server's response
        memset(output, 0, sizeof(output));
        while (recv(client_socket, output, sizeof(output), 0) > 0) {
            printf("%s", output);
            memset(output, 0, sizeof(output));
        }
    }

    // Close the socket
    close(client_socket);
    return 0;
}
