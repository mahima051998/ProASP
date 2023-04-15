#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>

#define MAX_CLIENTS 8
#define SERVER_PORT 3000

// function to process client requests
void *process_client(void *arg)
{
    // cast the argument to an integer that represents the client socket
    int client_socket = *(int *) arg;

    // buffer to store the client's command
    char command_buffer[1024];

    // file pointer to store the output of the command
    FILE *command_output;

    // continuously receive and process client commands until "quit" is received
    while (1)
    {
        // clear the command buffer and receive client's command
        memset(command_buffer, 0, sizeof(command_buffer));
        int bytes_received = recv(client_socket, command_buffer, sizeof(command_buffer), 0);

        // handle any receive errors
        if (bytes_received < 0)
        {
            perror("recv failed");
            exit(EXIT_FAILURE);
        }

        // check if the command is "quit"
        if (strcmp(command_buffer, "quit") == 0)
        {
            break;
        }

        // run the command using popen and store the output in command_output
        command_output = popen(command_buffer, "r");

        // handle any popen errors
        if (command_output == NULL)
        {
            perror("popen failed");
            exit(EXIT_FAILURE);
        }

        // clear the command buffer and send the output of the command to the client
        memset(command_buffer, 0, sizeof(command_buffer));
        while (fgets(command_buffer, sizeof(command_buffer), command_output) != NULL)
        {
            int bytes_sent = send(client_socket, command_buffer, strlen(command_buffer), 0);

            // handle any send errors
            if (bytes_sent < 0)
            {
                perror("send failed");
                exit(EXIT_FAILURE);
            }

            // clear the command buffer
            memset(command_buffer, 0, sizeof(command_buffer));
        }

        // close the command_output file pointer
        pclose(command_output);
    }

    // close the client socket and return
    close(client_socket);
    return NULL;
}

int main()
{
	// Declare variables for server and mirror sockets, and the server and client addresses
    int server_sock, mirror_sock;
    struct sockaddr_in server_addr, mirror_addr, client_addr;
    socklen_t addr_len = sizeof(server_addr);

    // Connect to the server
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
	// Set the server address information
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);
    memset(server_addr.sin_zero, 0, sizeof(server_addr.sin_zero));

	// Check if connection to the server was successful
    if (connect(server_sock, (struct sockaddr *)&server_addr, addr_len) == -1)
    {
        perror("connect failed");
        exit(EXIT_FAILURE);
    }

    // Connect to the mirror server
    mirror_sock = socket(AF_INET, SOCK_STREAM, 0);
    mirror_addr.sin_family = AF_INET;
    mirror_addr.sin_port = htons(MIRROR_PORT);
    mirror_addr.sin_addr.s_addr = inet_addr(MIRROR_ADDRESS);
    memset(mirror_addr.sin_zero, 0, sizeof(mirror_addr.sin_zero));

	// Check if connection to the mirror server was successful
    if (connect(mirror_sock, (struct sockaddr *)&mirror_addr, addr_len) == -1)
    {
        perror("connect failed");
        exit(EXIT_FAILURE);
    }

	// Declare variables for connection count and an array of sockets
    int conn_count = 0;
    int sockets[MAX_CONNECTIONS];
	// Add the server and mirror sockets to the array
    sockets[0] = server_sock;
    sockets[1] = server_sock;
    sockets[2] = server_sock;
    sockets[3] = server_sock;
    sockets[4] = mirror_sock;
    sockets[5] = mirror_sock;
    sockets[6] = mirror_sock;
    sockets[7] = mirror_sock;
	
	// Continuously accept incoming connections
    while (1)
    {
		// Accept a new connection from a client
        int conn = accept(server_sock, (struct sockaddr *)&client_addr, &addr_len);
        conn_count++;

		// Check if the connection should be handled by the server or mirror
        if (conn_count <= 4) // First 4 connections that handled by the server
        {
			// Create a new thread to process the client request
            pthread_t thread;
            pthread_create(&thread, NULL, processclient, (void *)&conn);
        }
        else if (conn_count <= 8) // Next 4 connections are handled by the mirror
        {
			// Create a new socket to connect to the mirror server
            int mirror_sock = socket(AF_INET, SOCK_STREAM, 0);
			// Set the mirror server address information
            struct sockaddr_in mirror_addr;
            memset(&mirror_addr, 0, sizeof(mirror_addr));
            mirror_addr.sin_family = AF_INET;
            mirror_addr.sin_port = htons(3001);
            mirror_addr.sin_addr.s_addr = inet_addr("mirror_ip_address");
			
			// Check if connection to the mirror server was successful
            if (connect(mirror_sock, (struct sockaddr *)&mirror_addr, sizeof(mirror_addr)) == -1)
            {
                perror("mirror connection failed");
                exit(EXIT_FAILURE);
            }
			
			// Send a message to the mirror to start processing the connection
            send(mirror_sock, "start_mirror", strlen("start_mirror"), 0);
			
			// Create a new thread to process the client request using the mirror socket
            pthread_t thread;
            pthread_create(&thread, NULL, processclient, (void *)&mirror_sock);
        }
		else // Handle remaining connections in alternating manner
	{
    if (conn_count % 2 != 0) // Odd connections are handled by the server
    {
        pthread_t server_thread;
        pthread_create(&server_thread, NULL, processclient, (void *)&conn);
    }
	// Even connections are handled by the mirror
    else 
    {
		// Create a socket for the mirror
        int mirror_sock = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in mirror_addr;
        memset(&mirror_addr, 0, sizeof(mirror_addr));
        mirror_addr.sin_family = AF_INET;
        mirror_addr.sin_port = htons(3001);
        mirror_addr.sin_addr.s_addr = inet_addr("mirror_ip_address");
		
		 // Connect to the mirror
        if (connect(mirror_sock, (struct sockaddr *)&mirror_addr, sizeof(mirror_addr)) == -1)
        {
            perror("mirror connection failed");
            exit(EXIT_FAILURE);
        }

		// Send a message to start the mirror
        send(mirror_sock, "start_mirror", strlen("start_mirror"), 0);
		
		// Create a thread to handle the mirror connection
        pthread_t mirror_thread;
        pthread_create(&mirror_thread, NULL, processclient, (void *)&mirror_sock);
			}
		}

      }
}
