#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>

char *validate(char *cmd) {
    char *res = malloc(sizeof(char) * 1024);
    char **resArray = malloc(sizeof(char*) * 2);
    int i;
    
    for (i = 0; i < 2; i++) {
        resArray[i] = malloc(sizeof(char) * 256);
    }
    
    if (sscanf(cmd, "findfile %ms", &resArray[0]) == 1) {
        sprintf(res, "echo -e $(find ~/ -maxdepth 1 -type f -name %s -exec stat -c '%%s\\t%%w\\t%%n' {} \\; | head -n 1)", resArray[0]);
        free(resArray[0]);
    }
    else if (sscanf(cmd, "sgetfiles %ms %ms%*c", &resArray[0], &resArray[1]) == 2) {
        sprintf(res, "zip temp.tar.gz $(find ~/ -maxdepth 1 -type f -size +%sc -size -%sc)", resArray[0], resArray[1]);
        free(resArray[0]);
        free(resArray[1]);
    }
    else if (sscanf(cmd, "dgetfiles %ms %ms%*c", &resArray[0], &resArray[1]) == 2) {
        sprintf(res, "zip temp.tar.gz $(find ~/ -maxdepth 1 -type f -newermt \"%s\" ! -newermt \"%s\")", resArray[0], resArray[1]);
        free(resArray[0]);
        free(resArray[1]);
    }
    else if (sscanf(cmd, "getfiles %[^\n]", res) == 1 || sscanf(cmd, "gettargz %[^\n]", res) == 1) {
        char *token;
        char files[1024] = "";
        int count = 0;
        token = strtok(res, " ");
        while (token != NULL) {
            char tmp[256];
            if (count > 0) {
                strcat(files, "-o ");
            }
            if (strcmp(cmd, "getfiles") == 0) {
                sprintf(tmp, "-name %s ", token);
            }
            else if (strcmp(cmd, "gettargz") == 0) {
                sprintf(tmp, "-iname \"*.%s\" ", token);
            }
            strcat(files, tmp);
            count++;
            token = strtok(NULL, " ");
        }
        sprintf(res, "zip temp.tar.gz $(find ~/ -maxdepth 1 -type f %s)", files);
    }
    else if (strcmp(cmd, "quit") == 0) {
        sprintf(res, "%s", cmd);
    }
    else {
        sprintf(res, "Invalid Command");
    }

    for (i = 0; i < 2; i++) {
        free(resArray[i]);
    }
    free(resArray);
    
    return res;
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