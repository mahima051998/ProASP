#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>

// Define the maximum length of user command and server output
#define MAX_COMMAND_LENGTH 1024
#define MAX_OUTPUT_LENGTH 1024

int main()
{
    int client_socket; // Declare a client socket file descriptor
    struct sockaddr_in server_address; // Declare a server address struct
    char user_command[MAX_COMMAND_LENGTH]; // Declare a buffer to store user command input
    char server_output[MAX_OUTPUT_LENGTH]; // Declare a buffer to store server output
    char *result; // Declare a pointer to store the result of the validate function
    int flags, select_result; // Declare variables for storing socket flags and select results

    // Create a TCP socket with IPv4 address family and default protocol (0)
    client_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (client_socket == -1)
    {
        perror("socket failed"); // Print an error message if the socket creation fails
        exit(EXIT_FAILURE);
    }

    // Set up the server address
    memset(&server_address, 0, sizeof(server_address)); // Zero out the server address struct
    server_address.sin_family = AF_INET; // Set the address family to IPv4
    server_address.sin_port = htons(3000); // Set the port number to 3000 and convert it to network byte order
    server_address.sin_addr.s_addr = inet_addr("127.0.0.1"); // Set the IP address to localhost (127.0.0.1)

    // Connect to the server using the client socket and server address
    if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1)
    {
        perror("connect failed"); // Print an error message if the connection fails
        exit(EXIT_FAILURE);
    }

    // Set the socket to non-blocking mode
    flags = fcntl(client_socket, F_GETFL, 0); // Get the current socket flags
    fcntl(client_socket, F_SETFL, flags | O_NONBLOCK); // Set the socket to non-blocking mode

    // Enter the main loop for sending and receiving data
    while (1)
    {
        // Prompt the user to enter a command
        printf("Enter command: ");
        fgets(user_command, MAX_COMMAND_LENGTH, stdin); // Get user input from stdin

        // Remove the trailing newline character from the user input
        if (user_command[strlen(user_command) - 1] == '\n')
        {
            user_command[strlen(user_command) - 1] = '\0'; // Replace the newline character with a null terminator
        }

        // Call the process_command function to validate the user input
        result = process_command(user_command); // Pass the user input to the process_command function and store the result in the pointer

        // Send the validated command to the server using the client socket
        if (send(client_socket, result, strlen(result), 0) == -1)
        {
            perror("send failed"); // Print an error message if the send operation fails
            exit(EXIT_FAILURE);
        }

        // Use select to check for data received from the server
        fd_set read_fds; // Declare a file descriptor set for reading
        FD_ZERO(&read_fds); // Initialize the file descriptor set to zero
        FD_SET(client_socket, &read_fds); // Add the client socket to the file descriptor set
        struct timeval timeout; // Declare a timeval struct for the timeout value
        timeout.tv_sec = 1; // Set
		timeout.tv_usec = 0;

		select_result = select(client_socket + 1, &read_fds, NULL, NULL, &timeout); // wait until there is data to read or timeout occurs

		if (select_result == -1) // check for errors in select
		{
			perror("select failed"); // print an error message
			exit(EXIT_FAILURE); // exit the program with failure status
		}
		else if (select_result == 0) // if no data is available to read
		{
			// No data received from the server, continue to the next iteration
			continue;
		}
		else // if data is available to read
		{
		// Receive data from the server and print it to the console
		memset(server_output, 0, sizeof(server_output)); // initialize server_output to all zeroes
		while (recv(client_socket, server_output, sizeof(server_output), 0) > 0) // loop through all the data received from the server
		{
			printf("%s", server_output); // print the received data to the console
			memset(server_output, 0, sizeof(server_output)); // clear the buffer for the next iteration
		}
		}

		// Close the client socket
		if (close(client_socket) == -1) // check for errors in closing the socket
		{
			perror("close failed"); // print an error message
			exit(EXIT_FAILURE); // exit the program with failure status
		}
}
		return 0; // exit the program with success status
		}
