#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>

#define MAX_CONNECTIONS 8
#define SERVER_PORT 3000

void *processclient(void *arg)
{
    int conn = *((int *)arg);
    char buffer[1024];
    FILE *fp;

    while (1)
    {
        memset(buffer, 0, sizeof(buffer));
        recv(conn, buffer, sizeof(buffer), 0);

        if (strcmp(buffer, "quit") == 0)
        {
            break;
        }

        // Process the command and send the result to the client
        fp = popen(buffer, "r");
        if (fp == NULL)
        {
            perror("popen failed");
            exit(EXIT_FAILURE);
        }
        memset(buffer, 0, sizeof(buffer));
        while (fgets(buffer, sizeof(buffer), fp))
        {
            send(conn, buffer, strlen(buffer), 0);
            memset(buffer, 0, sizeof(buffer));
        }
        pclose(fp);
    }
    close(conn);
    return NULL;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>

#define SERVER_ADDRESS "127.0.0.1" // The IP address of the server
#define SERVER_PORT 3000           // The port number of the server
#define MIRROR_ADDRESS "127.0.0.1" // The IP address of the mirror server
#define MIRROR_PORT 4000           // The port number of the mirror server
#define MAX_CONNECTIONS 8          // The maximum number of simultaneous connections

int conn;

int main()
{
    int server_socket, mirror_socket;
    struct sockaddr_in server_addr, mirror_addr, client_addr;
    socklen_t addr_len = sizeof(server_addr);

    // Connect to the server
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);
    memset(server_addr.sin_zero, 0, sizeof(server_addr.sin_zero));

    if (connect(server_socket, (struct sockaddr *)&server_addr, addr_len) == -1)
    {
        perror("connect failed");
        exit(EXIT_FAILURE);
    }

    // Connect to the mirror server
    mirror_socket = socket(AF_INET, SOCK_STREAM, 0);
    mirror_addr.sin_family = AF_INET;
    mirror_addr.sin_port = htons(MIRROR_PORT);
    mirror_addr.sin_addr.s_addr = inet_addr(MIRROR_ADDRESS);
    memset(mirror_addr.sin_zero, 0, sizeof(mirror_addr.sin_zero));

    if (connect(mirror_socket, (struct sockaddr *)&mirror_addr, addr_len) == -1)
    {
        perror("connect failed");
        exit(EXIT_FAILURE);
    }

    int conn_count = 0;
    int sockets[MAX_CONNECTIONS];
    sockets[0] = server_socket;
    sockets[1] = server_socket;
    sockets[2] = server_socket;
    sockets[3] = server_socket;
    sockets[4] = mirror_socket;
    sockets[5] = mirror_socket;
    sockets[6] = mirror_socket;
    sockets[7] = mirror_socket;

    while (1)
    {
        conn = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);
        conn_count++;

        if (conn_count <= 4) // First 4 connections are handled by the server
        {
            pthread_t thread;
            pthread_create(&thread, NULL, processclient, (void *)&conn);
        }
        else if (conn_count <= 8) // Next 4 connections are handled by the mirror
        {
            int mirror_socket = socket(AF_INET, SOCK_STREAM, 0);
            struct sockaddr_in mirror_addr;
            memset(&mirror_addr, 0, sizeof(mirror_addr));
            mirror_addr.sin_family = AF_INET;
            mirror_addr.sin_port = htons(3001);
            mirror_addr.sin_addr.s_addr = inet_addr("mirror_ip_address");

            if (connect(mirror_socket, (struct sockaddr *)&mirror_addr, sizeof(mirror_addr)) == -1)
            {
                perror("mirror connection failed");
                exit(EXIT_FAILURE);
            }

            send(mirror_socket, "start_mirror", strlen("start_mirror"), 0);

            pthread_t thread;
            pthread_create(&thread, NULL, processclient, (void *)&mirror_socket);
        }
        else // Remaining connections are handled in alternating manner
        {
            if (conn_count % 2 != 0) // Odd connections are handled by the server
            {
                pthread_t thread;
                pthread_create(&thread, NULL, processclient, (void *)&conn);
            }
            else // Even connections are handled by the mirror
            {
                int mirror_socket = socket(AF_INET, SOCK_STREAM, 0);
                struct sockaddr_in mirror_addr;
                memset(&mirror_addr, 0, sizeof(mirror_addr));
                mirror_addr.sin_family = AF_INET;
                mirror_addr.sin_port = htons(3001);
                mirror_addr.sin_addr.s_addr = inet_addr("mirror_ip_address");

                if (connect(mirror_socket, (struct sockaddr *)&mirror_addr, sizeof(mirror_addr)) == -1)
                {
                    perror("mirror connection failed");
                    exit(EXIT_FAILURE);
                }

                send(mirror_socket, "start_mirror", strlen("start_mirror"), 0);

                pthread_t thread;
                pthread_create(&thread, NULL, processclient, (void *)&mirror_socket);
            }
        }
    }
}