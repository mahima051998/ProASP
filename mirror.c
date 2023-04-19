#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <pthread.h>

#define BUFFER_SIZE 1024
#define MAX_CONNECTIONS 4 // Maximum number of connections to handle

// Function to handle client connections
void *process_client(void *arg)
{
    int client_sock = *((int *)arg); // Cast argument to integer pointer and dereference to get client socket
    char buf[BUFFER_SIZE];
    FILE *fp;

    // Keep handling requests until connection is closed or client requests to quit
    while (1)
    {
        memset(buf, 0, BUFFER_SIZE);
        if (recv(client_sock, buf, BUFFER_SIZE, 0) <= 0)
        {
            break; // connection closed
        }

        if (strcmp(buf, "quit") == 0)
        {
            break; // client requested to quit
        }

        // Execute the command and send the result to the client
        fp = popen(buf, "r"); // Open a process to execute the command and read the output
        if (fp == NULL)
        {
            perror("popen failed");
            exit(EXIT_FAILURE);
        }

        memset(buf, 0, BUFFER_SIZE);
        while (fgets(buf, BUFFER_SIZE, fp))
        {
            send(client_sock, buf, strlen(buf), 0); // Send each line of output to the client
            memset(buf, 0, BUFFER_SIZE);
        }

        pclose(fp); // Close the process
    }

    close(client_sock); // Close the client socket
    return NULL;
}

int main()
{

			
	FILE *fp;	
   fp = fopen("mirror_count.txt", "w");	
   if (fp == NULL) {	
      printf("Error while opening file!");	
      return 1;	
   }	
   fprintf(fp, "%d", 0);	
   fclose(fp);
    int server_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    server_sock = socket(AF_INET, SOCK_STREAM, 0); // Create a socket for the server
    if (server_sock == -1)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(4000); // Set the port number for the server
    server_addr.sin_addr.s_addr = INADDR_ANY; // Bind to any available network interface
    memset(server_addr.sin_zero, 0, sizeof(server_addr.sin_zero));

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) // Bind the socket to the server address
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_sock, MAX_CONNECTIONS) == -1) // Listen for client connections
    {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    int conn_count = 0; // Counter for number of connections
    while (1)
    {
        int client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_len); // Accept a client connection
        if (client_sock == -1)
        {
            perror("accept failed");
            exit(EXIT_FAILURE);
        }

        if (conn_count < MAX_CONNECTIONS)
        {
            // Handle first 4 connections in separate threads
            pthread_t thread;
            if (pthread_create(&thread, NULL, handle_connection, (void *)&client_sock) != 0) // Create a new thread to handle the connection
            {
                perror("failed to create thread");
                exit(EXIT_FAILURE);
            }
            pthread_detach(thread); // Detach the thread to allow it to run independently
        }
        else
        {
            // Close connection if it's beyond the first 4
            close(client_sock);
        }

        conn_count++;
    }

    close(server_sock);
    return 0;
}
