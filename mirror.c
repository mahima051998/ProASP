#include<stdio.h>
#include<netdb.h>
#include<netinet/in.h>
#include<stdlib.h>
#include<string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <limits.h>
#include <time.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<unistd.h> // read(), write(), close()
#define MAX 1000
#define PORT 8081
#define SA struct sockaddr
int  serverCounter=0;
   
int search_file(const char *path, const char *filename, char *result, struct stat *result_stat);
void findfile(const char *filename, char *response);

int search_file(const char *path, const char *filename, char *result, struct stat *result_stat)
 {
    DIR *dir;
    struct dirent *entry;
    struct stat entry_stat;
    int found = 0;

    dir = opendir(path);
    if (!dir) {
        return 0;
    }

    while ((entry = readdir(dir)) != NULL) {
        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        if (lstat(full_path, &entry_stat) == -1) {
            continue;
        }

        if (S_ISDIR(entry_stat.st_mode)) {
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                found = search_file(full_path, filename, result, result_stat);
                if (found) {
                    break;
                }
            }
        } else if (S_ISREG(entry_stat.st_mode)) {
            if (strcmp(entry->d_name, filename) == 0) {
                strncpy(result, full_path, PATH_MAX);
                *result_stat = entry_stat;
                found = 1;
                break;
            }
        }
    }

    closedir(dir);
    return found;
}

void findfile(const char *filename, char *response) {
    char home_dir[PATH_MAX];
    struct stat file_stat;

    system("cd ~");
    getcwd(home_dir, sizeof(home_dir));

    if (search_file(home_dir, (char *)filename, response, &file_stat)) {
        time_t t = file_stat.st_mtime;
        struct tm lt;
        localtime_r(&t, &lt);
        char date_created[20];
        strftime(date_created, sizeof(date_created), "%Y-%m-%d %H:%M:%S", &lt);

        char file_info[MAX];
        snprintf(file_info, sizeof(file_info), "%s, %lld, %s", response, (long long)file_stat.st_size, date_created);
        strncpy(response, file_info, MAX);
    } else {
        strncpy(response, "File not found", MAX);
    }
}



// Function designed for chat between client and server.
void serviceClient(int connfd)
{
    char command[MAX];
    int n;
    char* error= "Error!!";
    dup2(connfd, 1);
    // infinite loop for chat
    for (;;) {
        bzero(command, MAX);
        
        //printf("\n%d\n", connfd);
   
        // read the message from client and copy it in buffer
        if(n=read(connfd, command, sizeof(command))){
        
        }
    }
}
   
// Driver function
int main(int argc, char* argv[])
{
    int sockfd, connfd, len, status, num=0;
    struct sockaddr_in servaddr, cli;
   
    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("socket creation failed...\n");
        exit(0);
    }
    else
        printf("Socket successfully created..\n");
    bzero(&servaddr, sizeof(servaddr));
   
    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);
    
    // Binding newly created socket to given IP and verification
    if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) {
        printf("socket bind failed...\n");
        exit(0);
    }
    else
        printf("Socket successfully binded..\n");
   
    // Now server is ready to listen and verification
    if ((listen(sockfd, 5)) != 0) {
        printf("Listen failed...\n");
        exit(0);
    }
    else
        printf("Server listening..\n");
    len = sizeof(cli);

    // Accept the data packet from client and verification
    while(1){
        // printf("%d\n",servaddr.sin_port);
        // printf("%d\n", loadBalanceCounter);
        connfd = accept(sockfd, (SA*)&cli, &len);
        printf("Connected to client\n");
        if (connfd < 0) {
            printf("server accept failed...\n");
            exit(0);
        }
        else if(!fork())
            serviceClient(connfd);
        close(connfd);
    }
    close(sockfd);
}
