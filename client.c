#include <stdio.h> /* These are the usual header files */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <pthread.h>
#include "linklist.h"

#define SERVER_ADDR "127.0.0.1"
#define SERVER_PORT 5550

int client_sock;
struct sockaddr_in server_addr;

void *getSearchFileName(void *vargp)
{
    char fileName[BUFF_SIZE];
    int bytes_sent;
    Message ms;
    printf("Welcome to P2P file tranfer application\n");
    printf("Input file name and press ENTER to search:\n");
    while (1)
    {
        memset(fileName, '\0', (strlen(fileName)) + 1);
        fgets(fileName, BUFF_SIZE, stdin);
        fileName[strlen(fileName) - 1] = '\0';

        if (!strcmp(fileName, ""))
        {
            printf("Goodbye\n");
            exit(0);
        }
        else
        {
            ms.ms_type = FIND;
            ms.dt_type = STRING;
            strcpy(ms.value.buff, fileName);
            bytes_sent = send(client_sock, &ms, sizeof(ms), 0);
            if (bytes_sent <= 0)
            {
                printf("Connection closed!\n");
                exit(0);
            }
                }
    }
}

void getFileLst(char **fileLst)
{
    struct dirent *de;
    DIR *dr = opendir(".");
    int i = 0;
    if (dr == NULL) // opendir returns NULL if couldn't open directory
    {
        printf("Could not open current directory");
        return;
    }
    while ((de = readdir(dr)) != NULL)
    {
        if (de->d_type == 8)
            strcpy(fileLst[i++], de->d_name);
        if (i >= MAX_LEN)
        {
            printf("out of array length\n");
            break;
        }
    }
    closedir(dr);
}

int main()
{
    struct dirent *de;
    DIR *dr;
    pthread_t thread_id;
    /* server's address information */
    char buff[BUFF_SIZE];
    int msg_len, bytes_sent, bytes_received;
    int i = 0;

    Message ms, send_msg;
    // Step 1: Construct socket
    client_sock = socket(AF_INET, SOCK_STREAM, 0);

    // Step 2: Specify server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_ADDR);

    // Step 3: Request to connect server
    if (connect(client_sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) < 0)
    {
        printf("\nError!Can not connect to sever! Client exit imediately! ");
        return 0;
    }

    pthread_create(&thread_id, NULL, getSearchFileName, NULL);

    while (1)
    {
        bytes_received = recv(client_sock, &ms, sizeof(ms), 0); // blocking
        if (bytes_received <= 0)
        {
            printf("\nConnection closed");
            break;
        }
        else
        {
            switch (ms.ms_type)
            {
            case UPDATE:
                send_msg.dt_type = STRING_LIST;
                send_msg.ms_type = RES_UPDATE;
                send_msg.size = 0;
                dr = opendir("./doc");
                int i = 0;
                if (dr == NULL) // opendir returns NULL if couldn't open directory
                {
                    printf("Could not open current directory");
                    break;
                }
                while ((de = readdir(dr)) != NULL)
                {
                    if (de->d_type == 8)
                        strcpy(send_msg.value.filelst[i++], de->d_name);
                    send_msg.size = i;
                    if (i >= MAX_LEN)
                    {
                        printf("out of array length\n");
                        break;
                    }
                }
                closedir(dr);
                bytes_sent = send(client_sock, &send_msg, sizeof(send_msg), 0);
                if (bytes_sent <= 0)
                {
                    printf("Connection closed!\n");
                    exit(0);
                }
                break;
            case RES_FIND:
                printf("Client list:\n");
                for (int i = 0; i < ms.size; i++)
                {
                    printf("    %s:%d\n", inet_ntoa(ms.value.addrLst[i].sin_addr), ntohs(ms.value.addrLst[i].sin_port));
                }
                break;

            default:
                break;
            }
        }
    }
    close(client_sock);
    return 0;
}
