#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>

char* validate(char* command) {
    char* result = malloc(sizeof(char) * 1024);
    char* res[2];
    int i;

    if (sscanf(command, "findfile %ms", &res[0]) == 1) {
        sprintf(result, "echo -e $(find ~/ -maxdepth 1 -type f -name %s -exec stat -c '%%s\\t%%w\\t%%n' {} \\; | head -n 1)", res[0]);
        free(res[0]);
    } else if (sscanf(command, "sgetfiles %ms %ms%*c", &res[0], &res[1]) == 2) {
        sprintf(result, "zip temp.tar.gz $(find ~/ -maxdepth 1 -type f -size +%sc -size -%sc)", res[0], res[1]);
        free(res[0]);
        free(res[1]);
    } else if (sscanf(command, "dgetfiles %ms %ms%*c", &res[0], &res[1]) == 2) {
        sprintf(result, "zip temp.tar.gz $(find ~/ -maxdepth 1 -type f -newermt \"%s\" ! -newermt \"%s\")", res[0], res[1]);
        free(res[0]);
        free(res[1]);
    } else if (sscanf(command, "getfiles %[^\n]", result) == 1) {
        char files[1024] = "";
        char* token = strtok(result, " ");
        int count = 0;
        while (token != NULL) {
            char tmp[256];
            if (count > 0) {
                strcat(files, "-o ");
            }
            sprintf(tmp, "-name %s ", token);
            strcat(files, tmp);
            count++;
            token = strtok(NULL, " ");
        }
        sprintf(result, "zip temp.tar.gz $(find ~/ -maxdepth 1 -type f %s)", files);
    } else if (sscanf(command, "gettargz %[^\n]", result) == 1) {
        char files[1024] = "";
        char* token = strtok(result, " ");
        int count = 0;
        while (token != NULL) {
            char tmp[256];
            if (count > 0) {
                strcat(files, "-o ");
            }
            sprintf(tmp, "-iname \"*.%s\" ", token);
            strcat(files, tmp);
            count++;
            token = strtok(NULL, " ");
        }
        sprintf(result, "zip temp.tar.gz $(find ~/ -maxdepth 1 -type f %s)", files);
    } else if (strcmp(command, "quit") == 0) {
        sprintf(result, "%s", command);
    } else {
        sprintf(result, "Invalid Command");
    }

    return result;
}


int main()
{
    int client_socket;
    struct sockaddr_in server_addr;

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(3000);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    memset(server_addr.sin_zero, 0, sizeof(server_addr.sin_zero));

    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("connect failed");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        char command[1024];
        char output[1024];
        char *result;

        printf("Enter command: ");
        fgets(command, 1024, stdin);
        if (command[strlen(command) - 1] == '\n')
        {
            command[strlen(command) - 1] = '\0';
        }
        result = validate(command);

        send(client_socket, result, strlen(result), 0);

        memset(output, 0, sizeof(output));
        while (recv(client_socket, output, sizeof(output), 0) > 0)
        {
            printf("%s", output);
            memset(output, 0, sizeof(output));
        }
    }

    close(client_socket);
    return 0;
}
