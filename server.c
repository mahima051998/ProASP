#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>

// Define the server port number
#define PORT 3000

// Function to process client requests
void* process_client(void* arg) {
    // Cast the argument to an integer (client socket descriptor)
    int conn = *(int*)arg;
    char buffer[1024];
    FILE* fp;

    // Loop to receive and process client requests
    while (true) {
        // Clear the buffer and receive data from the client
        memset(buffer, 0, sizeof(buffer));
        if (recv(conn, buffer, sizeof(buffer), 0) <= 0)
            break;

        // Process the command and send the result to the client
        fp = popen(buffer, "r");
        if (fp == NULL) {
            perror("popen failed");
            exit(EXIT_FAILURE);
        }

        // Clear the buffer and send the result to the client
        memset(buffer, 0, sizeof(buffer));
        while (fgets(buffer, sizeof(buffer), fp)) {
            send(conn, buffer, strlen(buffer), 0);
            memset(buffer, 0, sizeof(buffer));
        }

        // Close the pipe and continue receiving client requests
        pclose(fp);
    }

    // Close the client socket and exit the thread
    close(conn);
    return NULL;
}

// Main function
int main() {

	FILE *fp;	
   fp = fopen("server_count.txt", "w");	
   if (fp == NULL) {	
      printf("Error while opening file!");	
      return 1;	
   }	
   fprintf(fp, "%d", 0);	
   fclose(fp);
   
    // Declare variables for the server and client sockets, and server and client addresses
    int server_socket, client_socket;
    struct sockaddr_in server_address, client_address;
    socklen_t address_length = sizeof(client_address);

    // Create a TCP socket for the server
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Configure the server address and port
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    server_address.sin_addr.s_addr = INADDR_ANY;
    memset(server_address.sin_zero, 0, sizeof(server_address.sin_zero));

    // Bind the server socket to the server address and port
    if (bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) == -1) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming client connections
    if (listen(server_socket, 5) == -1) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    // Loop to accept and handle client connections
    while (true) {
        // Accept a client connection and create a new thread to handle the client request
        client_socket = accept(server_socket, (struct sockaddr*)&client_address, &address_length);
        printf("Server connection has been established with client\n");
        if (client_socket == -1) {
            perror("accept failed");
            exit(EXIT_FAILURE);
        }

        // Create a new thread and pass the client socket descriptor as an argument to the thread function
        pthread_t thread;
        int* client_socket_ptr = (int*)malloc(sizeof(*client_socket_ptr));
        *client_socket_ptr = client_socket;

        if (pthread_create(&thread, NULL, process_client, client_socket_ptr) != 0) {
            perror("pthread_create failed");
            exit(EXIT_FAILURE);
        }
    }

    // Close the server socket and exit the program
    close(server_socket);
    return 0;
}
